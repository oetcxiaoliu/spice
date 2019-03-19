/*
   Copyright (C) 2009 Red Hat, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "playback.h"
#include "utils.h"
#include "debug.h"

#define REING_SIZE_MS 160

WavePlayer::WavePlayer(uint32_t sampels_per_sec, uint32_t bits_per_sample, uint32_t channels)
    : _pcm (NULL)
    , _hw_params (NULL)
    , _sw_params (NULL)
{
    if (!init(sampels_per_sec, bits_per_sample, channels)) {
        cleanup();
        THROW("failed");
    }
}

void WavePlayer::cleanup()
{
    if (_pcm) {
        snd_pcm_close(_pcm);
    }

    if (_hw_params) {
        snd_pcm_hw_params_free(_hw_params);
    }

    if (_sw_params) {
        snd_pcm_sw_params_free(_sw_params);
    }
}

WavePlayer::~WavePlayer()
{
    cleanup();
}

bool WavePlayer::init(uint32_t sampels_per_sec,
                      uint32_t bits_per_sample,
                      uint32_t channels)
{
    const int frame_size = WavePlaybackAbstract::FRAME_SIZE;
    const char* pcm_device = "default";
    snd_pcm_format_t format;
    int err;

    switch (bits_per_sample) {
    case 8:
        format = SND_PCM_FORMAT_S8;
        break;
    case 16:
        format = SND_PCM_FORMAT_S16_LE;
        break;
    default:
        return false;
    }
    _sampels_per_ms = sampels_per_sec / 1000;

    if ((err = snd_pcm_open(&_pcm, pcm_device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0) {
        LOG_ERROR("cannot open audio playback device %s %s", pcm_device, snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_hw_params_malloc(&_hw_params)) < 0) {
        LOG_ERROR("cannot allocate hardware parameter structure %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_sw_params_malloc(&_sw_params)) < 0) {
        LOG_ERROR("cannot allocate software parameter structure %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_hw_params_any(_pcm, _hw_params)) < 0) {
        LOG_ERROR("cannot initialize hardware parameter structure %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_hw_params_set_rate_resample(_pcm, _hw_params, 1)) < 0) {
        LOG_ERROR("cannot set rate resample %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_hw_params_set_access(_pcm, _hw_params,
                                            SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        LOG_ERROR("cannot set access type %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_hw_params_set_rate(_pcm, _hw_params, sampels_per_sec, 0)) < 0) {
        LOG_ERROR("cannot set sample rate %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_hw_params_set_channels(_pcm, _hw_params, channels)) < 0) {
        LOG_ERROR("cannot set channel count %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_hw_params_set_format(_pcm, _hw_params, format)) < 0) {
        LOG_ERROR("cannot set sample format %s", snd_strerror(err));
        return false;
    }

    snd_pcm_uframes_t buffer_size;
    buffer_size = (sampels_per_sec * REING_SIZE_MS / 1000) / frame_size * frame_size;

    if ((err = snd_pcm_hw_params_set_buffer_size_near(_pcm, _hw_params, &buffer_size)) < 0) {
        LOG_ERROR("cannot set buffer size %s", snd_strerror(err));
        return false;
    }

    int direction = 1;
    snd_pcm_uframes_t period_size = (sampels_per_sec * 20 / 1000) / frame_size * frame_size;
    if ((err = snd_pcm_hw_params_set_period_size_near(_pcm, _hw_params, &period_size,
                                                      &direction)) < 0) {
        LOG_ERROR("cannot set period size %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_hw_params(_pcm, _hw_params)) < 0) {
        LOG_ERROR("cannot set parameters %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_sw_params_current(_pcm, _sw_params)) < 0) {
        LOG_ERROR("cannot obtain sw parameters %s", snd_strerror(err));
        return false;
    }

    err = snd_pcm_hw_params_get_buffer_size(_hw_params, &buffer_size);
    if (err < 0) {
        LOG_ERROR("unable to get buffer size for playback: %s", snd_strerror(err));
        return false;
    }

    direction = 0;
    err = snd_pcm_hw_params_get_period_size(_hw_params, &period_size, &direction);
    if (err < 0) {
        LOG_ERROR("unable to get period size for playback: %s", snd_strerror(err));
        return false;
    }

    err = snd_pcm_sw_params_set_start_threshold(_pcm, _sw_params,
                                                (buffer_size / period_size) * period_size);
    if (err < 0) {
        LOG_ERROR("unable to set start threshold mode for playback: %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_sw_params(_pcm, _sw_params)) < 0) {
        LOG_ERROR("cannot set software parameters %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_prepare(_pcm)) < 0) {
        LOG_ERROR("cannot prepare pcm device %s", snd_strerror(err));
        return false;
    }

    return true;
}

bool WavePlayer::write(uint8_t* frame)
{
    snd_pcm_sframes_t ret = snd_pcm_writei(_pcm, frame, WavePlaybackAbstract::FRAME_SIZE);
    if (ret < 0) {
        if (ret == -EAGAIN) {
            return false;
        }
        DBG(0, "err %s", snd_strerror(-ret));
        if (snd_pcm_recover(_pcm, ret, 1) == 0) {
            snd_pcm_writei(_pcm, frame, WavePlaybackAbstract::FRAME_SIZE);
        }
    }
    return true;
}

void WavePlayer::stop()
{
    //snd_pcm_drain(_pcm);
	snd_pcm_drop(_pcm);
    snd_pcm_prepare(_pcm);
}

bool WavePlayer::abort()
{
    return true;
}

uint32_t WavePlayer::get_delay_ms()
{
    ASSERT(_pcm);

    snd_pcm_sframes_t delay;

    if (snd_pcm_delay(_pcm, &delay) < 0) {
        return 0;
    }
    return delay / _sampels_per_ms;
}


Mixer::Mixer():_handle(NULL)
{
	if(!init()){
		mixer_cleanup();
		THROW("failed");
	}
}

Mixer::~Mixer()
{
	if(_handle){
		snd_mixer_close(_handle);
		_handle = NULL;
	}
}
void Mixer::mixer_cleanup()
{
	if(_handle){
		snd_mixer_close(_handle);
		_handle = NULL;
	}
}
bool Mixer::mixer_init(int index)
{
	int result = 0;
	char card[64] = {0};
	sprintf (card, "hw:%d",index);
	if((result =  snd_mixer_open(&_handle,0))<0){
		LOG_ERROR("snd_mixer_open error %s",snd_strerror(result));
		//mixer_cleanup();
		return false;
	}
	if((result =  snd_mixer_attach(_handle,card)) < 0){
		LOG_ERROR("snd_mixer_attach error %s",snd_strerror(result));
		//mixer_cleanup();
		return false;
	}
	//register the mixer
	if((result = snd_mixer_selem_register(_handle,NULL,NULL))<0){
		LOG_ERROR("snd_mixer_selem_register error, %s",snd_strerror(result));
		//mixer_cleanup(result);
		return false;
	}
	//load the mixer
	if ((result = snd_mixer_load(_handle)) <0){
		LOG_ERROR("snd_mixer_load error %s",snd_strerror(result));
		//mixer_cleanup();
		return false;
	}
	snd_mixer_elem_t *elem = 0;

    for(elem=snd_mixer_first_elem(_handle); elem; elem=snd_mixer_elem_next(elem))
    {
		if (strcmp (snd_mixer_selem_get_name(elem), "Master") == 0)
		{
			return true;
		}		
    }

	return false;
}

bool Mixer::init()
{
	int index = 0;
	while(1)
	{
		char tmp_name[10] = {0};
		sprintf (tmp_name, "%d", index);
		int i = snd_card_get_index(tmp_name);
		if(i < 0 || i >= 32) {
			return false;
		}
		if (mixer_init(index)){
			return true;
		}
		else {
			mixer_cleanup();
			index++;
			continue;
		}
	}
	return false;
}
void Mixer::mixer_volume_control(long leftv,long rightv)
{
	//LOG_ERROR("mixer volume control");
	snd_mixer_elem_t *elem = 0;

	for(elem=snd_mixer_first_elem(_handle); elem; elem=snd_mixer_elem_next(elem))
    {
		if (strcmp (snd_mixer_selem_get_name(elem), "PCM") == 0)
		{
			snd_mixer_selem_set_playback_volume_range(elem,0,100);
			snd_mixer_selem_set_playback_volume(elem,SND_MIXER_SCHN_FRONT_LEFT,100);
			snd_mixer_selem_set_playback_volume(elem,SND_MIXER_SCHN_FRONT_RIGHT,100);
			break;
		}
	}

	for(elem=snd_mixer_first_elem(_handle); elem; elem=snd_mixer_elem_next(elem))
    {
		if (strcmp (snd_mixer_selem_get_name(elem), "Master") == 0)
		{
			snd_mixer_selem_set_playback_volume_range(elem,0,100);
			snd_mixer_selem_set_playback_volume(elem,SND_MIXER_SCHN_FRONT_LEFT,leftv);
			snd_mixer_selem_set_playback_volume(elem,SND_MIXER_SCHN_FRONT_RIGHT,rightv);
			break;		
		}
	}

}

void Mixer::mixer_toggle_mute(int mute)
{
	int signal;
	if(mute == 0){
		signal =1;
	} else {
		signal = 0;
	}
	/*snd_mixer_elem_t * master_element;
	master_element = snd_mixer_first_elem(_handle);
	//LOG_ERROR("mute:%d",signal);
	for(int i = 0;i < SND_MIXER_SCHN_LAST;i++){
		snd_mixer_selem_set_playback_switch(master_element,(snd_mixer_selem_channel_id_t)i,signal);
	}*/

	const char *elems[4] = {"Master","Headphone","PCM", "Speaker"};
	for(int i=0; i<4; i++) {
		mixer_mute_selem(elems[i], signal);
	}
}

void Mixer::mixer_mute_selem(const char *name, int mute)
{
	snd_mixer_elem_t *elem = 0;

	for(elem=snd_mixer_first_elem(_handle); elem; elem=snd_mixer_elem_next(elem)) {
		if (strcmp (snd_mixer_selem_get_name(elem), name) == 0) {
			snd_mixer_selem_set_playback_switch_all(elem, mute);
			break;
		}
	}
}

