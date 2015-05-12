/** \file sdt.reverb~.c
 * Max external for the simulation of reverberating ambients
 * (and other diffusive phenomena).
 *
 * \author Stefano Baldan (stefanobaldan@iuav.it)
 *
 * This file is part of the 'Sound Design Toolkit' (SDT)
 * Developed with the contribution of the following EU-projects:
 * 2001-2003 'SOb' http://www.soundobject.org/
 * 2006-2009 'CLOSED' http://closed.ircam.fr/
 * 2008-2011 'NIW' http://www.niwproject.eu/
 * 2014-2017 'SkAT-VG http://www.skatvg.eu/
 *
 * Contacts: 
 * 	stefano.papetti@zhdk.ch
 * 	stefano.dellemonache@gmail.com
 *  stefanobaldan@iuav.it
 *
 * Complete list of authors (either programmers or designers):
 * 	Federico Avanzini (avanzini@dei.unipd.it)
 *	Nicola Bernardini (nicb@sme-ccppd.org)
 *	Gianpaolo Borin (gianpaolo.borin@tin.it)
 *	Carlo Drioli (carlo.drioli@univr.it)
 *	Stefano Delle Monache (stefano.dellemonache@gmail.com)
 *	Delphine Devallez
 *	Federico Fontana (federico.fontana@uniud.it)
 *	Laura Ottaviani
 *	Stefano Papetti (stefano.papetti@zhdk.ch)
 *	Pietro Polotti (pietro.polotti@univr.it)
 *	Matthias Rath
 *	Davide Rocchesso (roc@iuav.it)
 *	Stefania Serafin (sts@media.aau.dk)
 *  Stefano Baldan (stefanobaldan@iuav.it)
 *
 * The SDT is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * The SDT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the SDT; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *****************************************************************************/

#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"
#include "SDT/SDTCommon.h"
#include "SDT/SDTEffects.h"

typedef struct _reverb {
  t_pxobject ob;
  SDTReverb *reverb;
  double size, time, time1k;
} t_reverb;

static t_class *reverb_class = NULL;

void *reverb_new(t_symbol *s, long argc, t_atom *argv) {
  t_reverb *x = (t_reverb *)object_alloc(reverb_class);
  long maxDelay;
  
  if (x) {
    dsp_setup((t_pxobject *)x, 1);
    outlet_new(x, "signal");
    if (argc > 0 && atom_gettype(&argv[0]) == A_LONG) {
      maxDelay = atom_getlong(&argv[0]);
    }
    else {
      maxDelay = 44100;
    }
    x->reverb = SDTReverb_new(maxDelay);
  }
  return (x);
}

void reverb_free(t_reverb *x) {
  dsp_free((t_pxobject *)x);
  SDTReverb_free(x->reverb);
}

void reverb_assist(t_reverb *x, void *b, long m, long a, char *s) {
  if (m == ASSIST_INLET) { //inlet
    switch (a) {
      case 0:
        sprintf(s, "(signal): Input");
        break;
      case 1:
        sprintf(s, "size (float): Average room size in m");
        break;
      case 2:
        sprintf(s, "time (float): Reverberation time, in s (global)");
        break;
      case 3:
        sprintf(s, "time1k (float): Reverberation time, in s (at 1 kHz)");
        break;
      default:
        break;
    }
  } 
  else {
    sprintf(s, "(signal): Reverberated output");
  }
}

void reverb_size(t_reverb *x, void *attr, long ac, t_atom *av) {
    x->size = atom_getfloat(av);
    SDTReverb_setSize(x->reverb, x->size);
    SDTReverb_update(x->reverb);
}

void reverb_time(t_reverb *x, void *attr, long ac, t_atom *av) {
    x->time = atom_getfloat(av);
	SDTReverb_setTime(x->reverb, x->time);
	SDTReverb_update(x->reverb);
}

void reverb_time1k(t_reverb *x, void *attr, long ac, t_atom *av) {
    x->time1k = atom_getfloat(av);
	SDTReverb_setTime1k(x->reverb, x->time1k);
	SDTReverb_update(x->reverb);
}

t_int *reverb_perform(t_int *w) {
  t_reverb *x = (t_reverb *)(w[1]);
  t_float *in = (t_float *)(w[2]);
  t_float *out = (t_float *)(w[3]);
  int n = (int)w[4];
	
  while (n--) {
    *out++ = (float)SDTReverb_dsp(x->reverb, *in++);
  }

  return w + 5;
}

void reverb_dsp(t_reverb *x, t_signal **sp, short *count) {
  SDT_setSampleRate(sp[0]->s_sr);
  SDTReverb_setSize(x->reverb, x->size);
  SDTReverb_setTime(x->reverb, x->time);
  SDTReverb_setTime1k(x->reverb, x->time1k);
  SDTReverb_update(x->reverb);
  dsp_add(reverb_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void reverb_perform64(t_reverb *x, t_object *dsp64, double **ins, long numins,
                      double **outs, long numouts, long sampleframes,
                      long flags, void *userparam) {
  t_double *in = ins[0];
  t_double *out = outs[0];
  int n = sampleframes;
	
  while (n--) {
    *out++ = SDTReverb_dsp(x->reverb, *in++);
  }
}

void reverb_dsp64(t_reverb *x, t_object *dsp64, short *count, double samplerate,
                  long maxvectorsize, long flags) {
  SDT_setSampleRate(samplerate);
  SDTReverb_setSize(x->reverb, x->size);
  SDTReverb_setTime(x->reverb, x->time);
  SDTReverb_setTime1k(x->reverb, x->time1k);
  SDTReverb_update(x->reverb);
  object_method(dsp64, gensym("dsp_add64"), x, reverb_perform64, 0, NULL);
}

int C74_EXPORT main(void) {	
  t_class *c = class_new("sdt.reverb~", (method)reverb_new, (method)reverb_free,
                         (long)sizeof(t_reverb), 0L, A_GIMME, 0);
	
  class_addmethod(c, (method)reverb_dsp, "dsp", A_CANT, 0);
  class_addmethod(c, (method)reverb_dsp64, "dsp64", A_CANT, 0);
  class_addmethod(c, (method)reverb_assist, "assist",	A_CANT, 0);

  CLASS_ATTR_DOUBLE(c, "size", 0, t_reverb, size);
  CLASS_ATTR_DOUBLE(c, "time", 0, t_reverb, time);
  CLASS_ATTR_DOUBLE(c, "time1k", 0, t_reverb, time1k);
  
  CLASS_ATTR_FILTER_MIN(c, "size", 0.0);
  CLASS_ATTR_FILTER_MIN(c, "time", 0.000002);
  CLASS_ATTR_FILTER_MIN(c, "time1k", 0.000001);
  
  CLASS_ATTR_ACCESSORS(c, "size", NULL, (method)reverb_size);
  CLASS_ATTR_ACCESSORS(c, "time", NULL, (method)reverb_time);
  CLASS_ATTR_ACCESSORS(c, "time1k", NULL, (method)reverb_time1k);
  
  CLASS_ATTR_ORDER(c, "size", 0, "1");
  CLASS_ATTR_ORDER(c, "time", 0, "2");
  CLASS_ATTR_ORDER(c, "time1k", 0, "3");

  class_dspinit(c);
  class_register(CLASS_BOX, c);
  reverb_class = c;

  return 0;
}