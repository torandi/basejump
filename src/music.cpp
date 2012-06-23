#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include "music.hpp"
#include "globals.hpp"

#include <portaudio.h>
#include <iostream>
#include <vorbis/vorbisfile.h>
#include <pulse/pulseaudio.h>

int Music::pa_contexts = 0;
int Music::device_index = 3;
PaTime Music::device_latency;
pa_mainloop * Music::pulse_main;
short Music::hw_device[2];

void Music::initialize_pa() {
	PaError err = Pa_Initialize();
	print_pa_error("initialize", err);

		find_default_device();
}

void Music::find_default_device() {
	/*
	 * Here we use pulseaudio to find out which hardware device 
	 * is default. The reason that we do this is that portaudio only supports alsa,
	 * and when using puls through alsa we can't get current time
	 */
	pulse_main = pa_mainloop_new();
	pa_mainloop_api * m_api = pa_mainloop_get_api( pulse_main );
	pa_context * pa_context = pa_context_new( m_api , "Frobnicators Demo Engine");

	pa_context_set_state_callback(pa_context, Music::pulse_context_callback, NULL);
	pa_context_connect(pa_context, NULL, PA_CONTEXT_NOFLAGS, NULL);

	int retval;
	//Run the main loop
	pa_mainloop_run( pulse_main , &retval);

	printf("[Music] Mainloop exited with status %d\n", retval);

	const PaDeviceInfo * device_info;
	const PaHostApiInfo * host_info;

	for(int i=0; i < Pa_GetDeviceCount(); ++i) {
		device_info = Pa_GetDeviceInfo( i );
		host_info = Pa_GetHostApiInfo( device_info->hostApi );
		fprintf(verbose, "[Music] [ %d ] %s ( %s ) channels: %d\n", i, device_info->name, host_info->name, device_info->maxOutputChannels);
		if(device_index == i)
			device_latency = device_info->defaultLowOutputLatency;
	}

	pa_context_disconnect( pa_context );
	pa_context_unref( pa_context );

	pa_mainloop_free( pulse_main );
}

void Music::pulse_context_callback(pa_context * c, void * userdata) {
	switch (pa_context_get_state(c)) {
		case PA_CONTEXT_READY:
			{
				fprintf(verbose,"[Music] Connected to %s\n", pa_context_get_server( c ));
				if(!pa_context_is_local( c )) {
					printf("[Music] Pulse is not local, can't use hardware devices directly\n");
					pa_mainloop_quit ( pulse_main, -1 ); //If it is not local we can't use the hw devices directly
				} else {
					pa_context_get_server_info( c, &Music::pulse_server_info_callback, NULL );
				}
				break;
			}
		case PA_CONTEXT_FAILED:
			printf("[Music] Connection to pulse failed: %s\n", pa_strerror( pa_context_errno ( c ) ) );
			pa_mainloop_quit( pulse_main, -1 );
			break;
		default:
			//Nope
			break;
	}
}

void Music::pulse_server_info_callback (pa_context *c, const pa_server_info *i, void *userdata) {
	fprintf(verbose, "[Music] Default sink name: %s\n", i->default_sink_name);
	pa_context_get_sink_info_by_name(c, i->default_sink_name, &Music::pulse_sink_info_callback, NULL );
}

void Music::pulse_sink_info_callback (pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
	if(eol == 1)
		return;
	if(i->flags & PA_SINK_HARDWARE) {
		fprintf(verbose,"[Music] eol: %d Default sink is a hardware sink, driver: %s\n", eol, i->driver);
		const char * prop;

		prop = pa_proplist_gets(i->proplist, "device.api");

		if(strcmp(prop, "alsa") != 0) {
			printf("[Music] Device api is not alsa\n");
			pa_mainloop_quit( pulse_main, -1 );
			return;
		}

		//We must copy the data since the returned pointer is invalid after the subsequent call to proplist_get
		prop = pa_proplist_gets(i->proplist, "alsa.name");
		char device_name[strlen(prop)+1];
		memcpy(device_name, prop, (strlen(prop)+1)*sizeof(char));

		prop = pa_proplist_gets(i->proplist, "alsa.card_name");
		char card_name[strlen(prop)+1];
		memcpy(card_name, prop, (strlen(prop)+1)*sizeof(char));

		prop = pa_proplist_gets(i->proplist, "alsa.card");
		hw_device[0] = atoi(prop);
		prop = pa_proplist_gets(i->proplist, "alsa.device");
		hw_device[1] = atoi(prop);

		fprintf(verbose,"[Music] Default device: %s:  %s (hw:%d,%d)\n", card_name, device_name, hw_device[0], hw_device[1]);

		pa_mainloop_quit( pulse_main, 1 );
	} else {
		printf("[Music] Default sink is not a hardware sink\n");
		pa_mainloop_quit( pulse_main, -1 );
	}
}

void Music::terminate_pa() {
	PaError err = Pa_Terminate();
	print_pa_error("terminate", err);
}

void Music::print_pa_error(const char * context, const PaError &err) {
	if(err == paNoError) return;
	printf("[Music] Error in %s: %s\n", context, Pa_GetErrorText( err ));
	abort();
}

void Music::set_finished_callback(void (*callback)(void*), void * data) {
	finished_callback = callback;
	callback_data = data;
}


void Music::pa_finished(void *userData) {
	Music * m = (Music*) userData;

	m->stop_decode();

	m->playing = false;
	if(m->finished_callback != NULL) (*(m->finished_callback)) ( m->callback_data );
}

int Music::pa_callback(const void *inputBuffer,
													void *outputBuffer,
													unsigned long framesCount,
													const PaStreamCallbackTimeInfo* timeInfo,
													PaStreamCallbackFlags statusFlags,
													void *userData ) {
	Music * m = (Music*) userData;
	int16_t * next;
	int16_t * out = (int16_t*) outputBuffer;
	
	for(unsigned long i = 0; i < framesCount*m->num_channels; ++i) {
		next = m->next_ptr(m->buffer_read);
		if(next == m->buffer_write) {
			//Fill the rest with silence:
			for(;i < framesCount*m->num_channels; ++i) {
				*out++ = 0;
			}

			if(m->eof_reached) {
				return paComplete; //We have played all data
			} else {
				fprintf(stderr, "[Music] Critical error: No data left in buffer\n");
				return paAbort;
			}
		} else {
			*out++ = *(m->buffer_read);
			m->buffer_read = next;
		}
	}
	return paContinue;
}

Music::Music(const char * file, int buffer_size_) :
		buffer_size(buffer_size_)
	, playing(false) 
	, decode(false)
	, finished_callback(nullptr)
	, callback_data(nullptr) {
	if(pa_contexts == 0) initialize_pa();
	++pa_contexts;

	buffer = (int16_t*) malloc(sizeof(int16_t)*buffer_size);
	ogg_buffer = (char*) malloc(sizeof(char)*OGG_BUFFER_SIZE);

	char real_path[strlen(file)+strlen(PATH_MUSIC)+1];

	sprintf(real_path, "%s%s", PATH_MUSIC, file);

	load_ogg(real_path);

	PaStreamParameters params;
	params.channelCount = num_channels;
	params.device = device_index;
	params.hostApiSpecificStreamInfo = NULL;
	params.sampleFormat = paInt16;
	params.suggestedLatency = device_latency;

	PaError err = Pa_OpenStream(&stream,
			nullptr,
			&params,
			sample_rate,
			paFramesPerBufferUnspecified,
			paNoFlag,
			&Music::pa_callback,
			this);
	print_pa_error("create stream", err);
	err = Pa_SetStreamFinishedCallback(stream, &Music::pa_finished);
	print_pa_error("set finish callback", err);
}

Music::~Music() {
	PaError err = Pa_CloseStream( stream );
	print_pa_error("close stream", err);

	free(buffer);
	free(ogg_buffer);

	ov_clear(&ogg_file);

	--pa_contexts;
	if(pa_contexts == 0) terminate_pa();
}

void Music::play(int num_loops) {
	//The stream might just be inactive (due to eof)
	if( ! Pa_IsStreamStopped ( stream ) ) Pa_StopStream( stream );

	if(playing) {
		printf("[Music] Warning, called Music::play() on playing stream\n");
		return;
	}
	loops_remaining = num_loops;

	reset_ogg_position();
	buffer_write = buffer;
	buffer_read = buffer + buffer_size - 1;

	start_time = Pa_GetStreamTime(stream);

	eof_reached = false;
	playing = true;
	start_decode();
	Pa_StartStream( stream );
}

void Music::stop() {
	PaError err = Pa_StopStream( stream );
	print_pa_error("stop stream", err);
}

double Music::time() const {
	return Pa_GetStreamTime( stream ) - start_time;
}

void Music::reset_ogg_position() {
	int seek=ov_time_seek(&ogg_file,0);
	if(seek!=0) {
		switch(seek) {
			case OV_ENOSEEK:
					printf("Bitstream is not seekable.\n");
					break;
			case OV_EINVAL:
					printf("Invalid argument value; possibly called with an OggVorbis_File structure that isn't open.\n");
					break;
			case OV_EREAD:
					printf("A read from media returned an error.\n");
					break;
			case OV_EFAULT:
					printf("Internal logic fault; indicates a bug or heap/stack corruption.\n");
					break;
			case OV_EBADLINK:
					printf("Invalid stream section supplied to libvorbisfile, or the requested link is corrupt.");
					break;
		}
		abort();
	}
}

int16_t * Music::next_ptr(int16_t *ptr) const{
	int16_t * next = ptr+1;
	if(next >= (buffer+buffer_size) )
		return buffer;
	else
		return next;
}

void Music::load_ogg(const char * filename) {
	vorbis_info *pInfo;

	source = fopen(filename,"r");

	int status=ov_open(source,&ogg_file,NULL,0);

	if(status!=0) {
		switch(status) {
			case OV_EREAD:
				printf("[Music] Failed to read %s\n",filename);
				break;
			case OV_ENOTVORBIS:
				printf("[Music] %s is not a vorbis file.\n",filename);
				break;
			case OV_EVERSION:
				printf("[Music] %s have wrong vorbis version.\n",filename);
				break;
			case OV_EBADHEADER:
				printf("[Music] %s have invalid vorbis header.\n",filename);
				break;
			case OV_EFAULT:
				printf("[Music] Internal error when loading %s.\n",filename);
				break;
		}
		abort();
	}

	pInfo=ov_info(&ogg_file,-1);

	if(pInfo) {
		num_channels = pInfo->channels;

		sample_rate = (double) pInfo->rate;

		fprintf(verbose,"[Music] %s loaded. Channels: %d Freq: %f Hz \n",filename,num_channels, sample_rate);

	} else {
		fprintf(stderr,"[Music] Failed to get ov_info from file %s\n",filename);
	}
}

void Music::start_decode() {
	decode = true;
	pthread_create(&decoder_thread, NULL, &Music::decode_thread_helper, this);
}

void Music::stop_decode() {
	decode = false;
	printf("Waiting for decoder thread to terminate\n");
	pthread_join(decoder_thread, nullptr);
	printf("done\n");
}

void * Music::decode_thread_helper(void * data) {
	((Music*) data)->run_decode();
	pthread_exit(NULL);
	return nullptr;
}

void Music::run_decode() {
	while(decode) {
		if(!buffer_data()) {
			if(loops_remaining > 0) {
				reset_ogg_position();
				--loops_remaining;
			} else if (loops_remaining == -1) {
				reset_ogg_position();
			} else {
				eof_reached = true;
				decode = false;
				return;
			}
		}
	}
}

bool Music::buffer_data() {
	int bit_stream;
	//Decode data
	int bytes = ov_read(&ogg_file,ogg_buffer, OGG_BUFFER_SIZE, ENDIAN,2,1,&bit_stream);

	if(bytes < 0) {
		switch(bytes) {
			case OV_HOLE:
				fprintf(stderr, "[Music] Error in vorbis decode: data interruption (OV_HOLE)\n");
				abort();
			case OV_EBADLINK:
				fprintf(stderr, "[Music] Error in vorbis decode: invalid stream section (OV_EBADLINK)\n");
				abort();
			case OV_EINVAL:
				fprintf(stderr, "[Music] Error in vorbis decode: corrupt data (OV_EINVAL)\n");
				abort();
		}
		return false;
	} else if(bytes == 0) {
		return false;
	} else {
		//Move data to sound buffer:
		char * pos = ogg_buffer;
		while(decode && bytes > 0) {
			int16_t * next = next_ptr(buffer_write);
			while(decode && next == buffer_read) {
				usleep(OVERFILL_SLEEP);
			}
			if(!decode) return false;
			*buffer_write = *((int16_t*) pos); //Write
			buffer_write = next; //Advance pointer
			bytes -= sizeof(int16_t)/sizeof(char);
			pos += sizeof(int16_t)/sizeof(char);
		}
		return true;
	}
}
