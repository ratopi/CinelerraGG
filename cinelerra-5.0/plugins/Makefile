include ../global_config

# burn must come before any other effecttv plugin
# colors must come before any plugin
# parametric must come before fourier plugins
# motion must come before perspective

DIRS = \
	1080to480 \
	1080to540 \
	720to480 \
	aging \
	audioscope \
	bandslide \
	bandwipe \
	blondtheme \
	bluebanana \
	bluedottheme \
	blur \
	brightness \
	brighttheme \
	burn \
	C41 \
	cdripper \
	chromakey \
	chromakeyhsv \
	color3way \
	colorbalance \
	compressor \
	crossfade \
	dcoffset \
	decimate \
	deinterlace \
	deinterlace-cv \
	delayaudio \
	delayvideo \
	denoise \
	denoisefft \
	denoisemjpeg \
	denoiseseltempavg \
	denoisevideo \
	despike \
	diffkey \
	dissolve \
	dot \
	downsample \
	echo \
	echocancel \
	fieldframe \
	flash \
	flip \
	framefield \
	freeverb \
	freezeframe \
	gain \
	gamma \
	gradient \
	graphic \
	greycstoration \
	histogram \
	holo \
	huesaturation \
	interpolate \
	interpolateaudio \
	interpolatevideo \
	invertaudio \
	invertvideo \
	irissquare \
	ivtc \
	lens \
	level \
	libeffecttv \
	linearblur \
	liveaudio \
	livevideo \
	loopaudio \
	loopvideo \
	motion \
	motion2point \
	motionblur \
	normalize \
	oilpainting \
	overlay \
	overlayaudio \
	parametric \
	perspective \
	photoscale \
	pitch \
	polar \
	radialblur \
	reframe \
	reframert \
	removegaps \
	reroute \
	resample \
	resamplert \
	reverb \
	reverseaudio \
	reversevideo \
	rgb601 \
	rgbshift \
	rotate \
	scale \
	shapewipe \
	sharpen \
	shiftinterlace \
	slide \
	spectrogram \
	suv \
	svg \
	swapchannels \
	swapframes \
	synthesizer \
	threshold \
	timeavg \
	timefront \
	timestretch \
	timestretchrt \
	titler \
	translate \
	unsharp \
	videoscope \
	vocoder \
	wave \
	whirl \
	wipe \
	yuv \
	yuvshift \
	zoom \
	zoomblur \

#	findobject \

DATA = $(PLUGIN_DIR)/fonts $(PLUGIN_DIR)/shapes
MPEG2ENC = $(PLUGIN_DIR)/mpeg2enc.plugin
HVEG2ENC = $(PLUGIN_DIR)/hveg2enc.plugin
LADSPA = $(PLUGIN_DIR)/ladspa

all:	$(DIRS) $(DATA) $(MPEG2ENC) $(HVEG2ENC) $(LADSPA)

$(DATA):
	cp -a $(notdir $@) $(PLUGIN_DIR)/.

$(MPEG2ENC):
	cp -a ../thirdparty/mjpegtools*/mpeg2enc/mpeg2enc $@

$(HVEG2ENC):
	cp -a ../mpeg2enc/$(OBJDIR)/hveg2enc $@

$(LADSPA):
	mkdir -p $@
	if [ -d $(THIRDPARTY)/ladspa-*/usr/local/lib/ladspa ]; then \
		cp -a $(THIRDPARTY)/ladspa-*/usr/local/lib/ladspa/. $@/. ; \
	fi

# parallel compilation of all directories
.PHONY:	$(DIRS)
$(DIRS):
	$(MAKE) -C $@

clean:
	rm -rf $(foreach d,$(DIRS),$(d)/$(OBJDIR))
	rm -rf $(PLUGIN_DIR)

install:

# dependencies for parallel build
aging:		libeffecttv
burn:		libeffecttv
dot:		libeffecttv
holo:		libeffecttv
motion2point:	downsample motion
motion:		downsample
timestretchrt:	timestretch

