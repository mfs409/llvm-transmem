# This file provides the names of all of our TM algorithms, in a format that 
# can be used by Makefiles

# For the STM algorithms, we are currently experimenting with the C++ memory
# model.  "Unsafe" versions are not correct wrt the C++ memory model; their
# "safe" counterparts remedy the problems.
STM_NAMES = cgl_mutex cohorts htm_gl tlrw_eager                           \
            hybrid_norec_two_counter_safe hybrid_norec_two_counter_unsafe \
            norec_quiescence_safe         norec_quiescence_unsafe         \
            orec_eager_quiescence_safe    orec_eager_quiescence_unsafe    \
            orec_lazy_quiescence_safe     orec_lazy_quiescence_unsafe     \
            orec_mixed_quiescence_safe    orec_mixed_quiescence_unsafe    \
            reduced_hardware_norec_safe   reduced_hardware_norec_unsafe   \
            hybrid_ring_sw_safe           hybrid_ring_sw_unsafe           \
            ring_mw_safe                  ring_mw_unsafe                  \
            ring_sw_safe                  ring_sw_unsafe                  \
            tl2_quiescence_safe           tl2_quiescence_unsafe           \
            tml_eager_safe                tml_eager_unsafe                \
            tml_lazy_safe                 tml_lazy_unsafe                 \
            rdtscp_lazy

# For the PTM algorithms, we are currently investigating different levels of
# dynamic optimization, which depend on what guarantees the program can
# provide.  "pn" are persistent-naive, "pg" are persistent-general (general
# programming model), and "pi" are persistent-ideal (ideal programming
# model).
PTM_NAMES = pn_cgl_eager  pg_cgl_eager  pi_cgl_eager  \
            pn_cgl_lazy   pg_cgl_lazy   pi_cgl_lazy   \
            pn_norec      pg_norec      pi_norec      \
            pn_orec_lazy  pg_orec_lazy  pi_orec_lazy  \
            pn_orec_mixed pg_orec_mixed pi_orec_mixed \
            pn_ring_sw    pg_ring_sw    pi_ring_sw    \
            pn_ring_mw    pg_ring_mw    pi_ring_mw    \
            pn_orec_eager pg_orec_eager pi_orec_eager \
            pn_tlrw_eager pg_tlrw_eager pi_tlrw_eager \
            pn_tl2        pg_tl2        pi_tl2

TM_LIB_NAMES = $(STM_NAMES) $(PTM_NAMES)
