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

TM_LIB_NAMES = $(STM_NAMES)
