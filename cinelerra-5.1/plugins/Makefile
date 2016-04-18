include ../global_config

ifeq ($(HAVE_DVB), y)
LIVEDVB = liveaudio livevideo
endif

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
	bluebanana \
	blur \
	brightness \
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
	edge \
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
	histogram_bezier \
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
	$(LIVEDVB) \
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
	theme_blond \
	theme_blond_cv \
	theme_blue \
	theme_blue_dot \
	theme_bright \
	theme_hulk \
	theme_pinklady \
	theme_suv \
	theme_unflat \

#	findobject \

DATA = $(PLUGIN_DIR)/fonts $(PLUGIN_DIR)/shapes
LADSPA = $(BINDIR)/ladspa

all:	$(DIRS) $(DATA) $(LADSPA)

$(DATA):
	cp -a $(notdir $@) $(PLUGIN_DIR)/.

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

