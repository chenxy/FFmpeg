# use pkg-config for getting CFLAGS and LDLIBS
FFMPEG_LIBS=    libavdevice                        \
                libavformat                        \
                libavfilter                        \
                libavcodec                         \
                libswresample                      \
                libswscale                         \
                libavutil                          \

CFLAGS += -Wall -O2 -g
CFLAGS += $(shell pkg-config --cflags $(FFMPEG_LIBS))
LDLIBS += $(shell pkg-config --libs $(FFMPEG_LIBS))

EXAMPLES=       decoding_encoding                  \
                filtering_video                    \
                filtering_audio                    \
                metadata                           \
                muxing                             \
                scaling_video                      \

OBJS=$(addsuffix .o,$(EXAMPLES))

# the following examples make explicit use of the math library
decoding_encoding: LDLIBS += -lm
muxing:            LDLIBS += -lm

.phony: all clean-test clean

all: $(OBJS) $(EXAMPLES)

clean-test:
	$(RM) test*.pgm test.h264 test.mp2 test.sw test.mpg outscale*.pgm

clean: clean-test
	$(RM) $(EXAMPLES) $(OBJS)
