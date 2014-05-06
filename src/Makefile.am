bin_PROGRAMS = kal
USRP_LIBS = -lbladeRF

kal_SOURCES = \
   arfcn_freq.cc \
   c0_detect.cc	 \
   circular_buffer.cc \
   fcch_detector.cc \
   kal.cc \
   offset.cc \
   bladeRF_source.cc \
   util.cc\
   arfcn_freq.h \
   c0_detect.h \
   circular_buffer.h \
   fcch_detector.h \
   offset.h \
   bladeRF_complex.h \
   bladeRF_source.h \
   util.h\
   version.h

kal_CXXFLAGS = $(FFTW3_CFLAGS) $(USRP_CFLAGS)
kal_LDADD = $(FFTW3_LIBS) $(USRP_LIBS)
