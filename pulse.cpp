#include <thread>
#include <mutex>
#include <condition_variable>
#include <pulse/simple.h>
#include <pulse/error.h>

class PulseAudioSimple {
    pa_simple* s;

public:
    PulseAudioSimple()
    {
        static const pa_sample_spec ss{PA_SAMPLE_S16LE, 44100, 2};
        int error;
        s = pa_simple_new(nullptr, APP_NAME, PA_STREAM_PLAYBACK, nullptr, "playback", &ss, nullptr, nullptr, &error);
        if (s==nullptr) {
            std::cerr << "Could not connect to pulseaudio server: " << pa_strerror(error) << "\n";
        }
    }

    ~PulseAudioSimple()
    {
        if (s!=nullptr) {
            pa_simple_free(s);
        }
    }

    bool is_ok() { return s!=nullptr; }

    bool write(std::vector<uint8_t> buffer)
    {
        int error = 0;
        if (pa_simple_write(s, buffer.data(), buffer.size(), &error)<0) {
            std::cerr << "pa_simple_write() failed: " << pa_strerror(error) << "\n";
            return false;
        }
        return true;
    }

    bool drain()
    {
        int error = 0;
        if (pa_simple_drain(s, &error)<0) {
            std::cerr << "pa_simple_drain() failed: " << pa_strerror(error) << "\n";
            return false;
        }
        return true;
    }
};

class PulseAudioThread {
    std::mutex audio_mutex;
    std::condition_variable audio_cv;
    std::thread thread;

    enum class AudioState {
        Waiting,
        Sleeping,
        Beeping,
        Exit,
    };
    AudioState audio_state = AudioState::Waiting;

    void audio_thread_main()
    {
        std::vector<u_int8_t> beep_file = read_entire_file(BEEP_FILE);
        const int wav_header_size = 44;
        if (beep_file.size()<wav_header_size)
            throw std::runtime_error("not a wav file");
        std::vector<u_int8_t> beep_data{beep_file.begin()+wav_header_size, beep_file.end()};

        std::optional<PulseAudioSimple> audio;
        while (true) {
            std::unique_lock<std::mutex> lock(audio_mutex);
            switch (audio_state) {
            case AudioState::Waiting:
                audio_cv.wait(lock);
                break;
            case AudioState::Sleeping:
                if (audio) {
                    audio->drain();
                    audio.reset();
                }
                audio_cv.wait(lock);
                break;
            case AudioState::Beeping:
                audio_state = AudioState::Waiting;
                lock.unlock();
                if (!audio)
                    audio.emplace();
                if (!(audio->is_ok() && audio->write(beep_data))) { // do not drain here to avoid clicking
                    stdout_beep();
                }
                break;
            case AudioState::Exit:
                if (audio) {
                    audio->drain();
                }
                return;
            }
        }
    }

    void set_state(AudioState state)
    {
        std::unique_lock<std::mutex> lock(audio_mutex);
        if (audio_state!=state) {
            audio_state = state;
            audio_cv.notify_all();
        }
    }

public:
    PulseAudioThread()
            :audio_mutex(), audio_cv()
    {
        thread = std::thread{[&]() { audio_thread_main(); }};
    }

    ~PulseAudioThread() { join(); }

    void sleep() { set_state(AudioState::Sleeping); }

    void beep() { set_state(AudioState::Beeping); }

    void exit() { set_state(AudioState::Exit); }

    void join()
    {
        exit();
        thread.join();
    }
};
