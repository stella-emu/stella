;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;                                                                       ;;
;;;                        Nicholas Ing-Simmons                           ;;
;;;                         Copyright (c) 2002                            ;;
;;;                        All Rights Reserved.                           ;;
;;;                                                                       ;;
;;;  Permission is hereby granted, free of charge, to use and distribute  ;;
;;;  this software and its documentation without restriction, including   ;;
;;;  without limitation the rights to use, copy, modify, merge, publish,  ;;
;;;  distribute, sublicense, and/or sell copies of this work, and to      ;;
;;;  permit persons to whom this work is furnished to do so, subject to   ;;
;;;  the following conditions:                                            ;;
;;;   1. The code must retain the above copyright notice, this list of    ;;
;;;      conditions and the following disclaimer.                         ;;
;;;   2. Any modifications must be clearly marked as such.                ;;
;;;   3. Original authors' names are not deleted.                         ;;
;;;   4. The authors' names are not used to endorse or promote products   ;;
;;;      derived from this software without specific prior written        ;;
;;;      permission.                                                      ;;
;;;                                                                       ;;
;;;  THE UNIVERSITY OF EDINBURGH AND THE CONTRIBUTORS TO THIS WORK        ;;
;;;  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      ;;
;;;  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   ;;
;;;  SHALL THE UNIVERSITY OF EDINBURGH NOR THE CONTRIBUTORS BE LIABLE     ;;
;;;  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    ;;
;;;  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   ;;
;;;  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          ;;
;;;  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       ;;
;;;  THIS SOFTWARE.                                                       ;;
;;;                                                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;;  Support for rsynth as an external module.
;;;   Derived from the handler for mbrola by Nick Ing-Simmons
;;;

;;; You might want to set this in your sitevars.scm
(defvar rsynth_progname "/usr/local/bin/say"
  "rsynth_progname
  The program name for rsynth.")
(defvar rsynth_options "-E -v -a"
  "rsynth_options
 Command line options to be passed during rsynth Synthesis.")

(define (rsynth_Synth utt)
  "(rsynth_Synth UTT)
  Synthesize using rsynth as external module.  Basically dump the info
  from this utterance. Call rsynth and reload the waveform into utt.
  [see rsynth]"
  (let ((filename (make_tmp_filename)))
    (save_segments_rsynth utt filename)
    (system (string-append rsynth_progname " " 
                           rsynth_options
			   " -o " filename ".au"
			   " -f " filename
			   ))
    (utt.import.wave utt (string-append filename ".au"))
    (apply_hooks after_synth_hooks utt)
    (delete-file filename)
    (delete-file (string-append filename ".au"))
    utt))

(define (save_segments_rsynth utt filename)
  "(save_segments_rsynth UTT FILENAME)
  Save segment information in rsynth format in filename.  The format is
  phone duration (ms) [% position F0 target]*. [see rsynth]"
  (let ((fd (fopen filename "w")))
    (mapcar
     (lambda (segment) 
       (save_seg_rsynth_entry 
	(item.feat segment 'name)
	(item.feat segment 'segment_start)
	(item.feat segment 'segment_duration)
	(mapcar
	 (lambda (targ_item)
	   (list
	    (item.feat targ_item "pos")
	    (item.feat targ_item "f0")))
	 (item.relation.daughters segment 'Target)) ;; list of targets
	fd))
     (utt.relation.items utt 'Segment))
    (fclose fd)))

(define (save_seg_rsynth_entry name start dur targs fd)
  "(save_seg_rsynth_entry ENTRY NAME START DUR TARGS FD)
  Entry contains, (name duration num_targs start 1st_targ_pos 1st_targ_val)."
  (format fd "%s %d " name (nint (* dur 1000)))
  (if targs     ;; if there are any targets
      (mapcar
       (lambda (targ) ;; targ_pos and targ_val
	 (let ((targ_pos (car targ))
	       (targ_val (car (cdr targ))))
	                                  
	   (format fd "%d %d " 
		   (nint (* 100 (/ (- targ_pos start) dur))) ;; % pos of target
		   (nint (parse-number targ_val)))           ;; target value
	   ))
       targs))
  (terpri fd)
  (terpri fd)
)
	
(provide 'rsynth)
