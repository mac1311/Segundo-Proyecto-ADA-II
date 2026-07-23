// ============================================================
// Timer.h
// Utilidad para medir tiempos de ejecucion con alta precision
// Usa std::chrono::high_resolution_clock internamente
// ============================================================

#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>

class Timer {
public:
    using Clock     = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    Timer() : running_(false), elapsed_ns_(0) {}

    // Inicia (o reinicia) el cronometro
    void start() {
        elapsed_ns_ = 0;
        running_    = true;
        t0_         = Clock::now();
    }

    // Detiene el cronometro y acumula el tiempo transcurrido
    void stop() {
        if (running_) {
            TimePoint t1 = Clock::now();
            elapsed_ns_ += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0_).count();
            running_ = false;
        }
    }

    // Devuelve el tiempo transcurrido en nanosegundos
    long long nanoseconds() const {
        if (running_) {
            TimePoint now = Clock::now();
            return elapsed_ns_ +
                   std::chrono::duration_cast<std::chrono::nanoseconds>(now - t0_).count();
        }
        return elapsed_ns_;
    }

    // Devuelve el tiempo transcurrido en microsegundos
    double microseconds() const { return nanoseconds() / 1000.0; }

    // Devuelve el tiempo transcurrido en milisegundos (el mas util para el solver)
    double milliseconds() const { return nanoseconds() / 1'000'000.0; }

    // Devuelve el tiempo transcurrido en segundos
    double seconds() const { return nanoseconds() / 1'000'000'000.0; }

    // Devuelve una cadena con el tiempo formateado de forma inteligente:
    //   < 1 ms  -> "X.XXX us"
    //   < 10 s  -> "X.XXX ms"
    //   >= 10 s -> "X.XXX s"
    std::string formatted() const {
        double us = microseconds();
        double ms = milliseconds();
        double s  = seconds();

        std::ostringstream oss;
        oss << std::fixed;

        if (us < 1000.0) {
            oss << std::setprecision(3) << us << " us";
        } else if (s < 10.0) {
            oss << std::setprecision(3) << ms << " ms";
        } else {
            oss << std::setprecision(3) << s << " s";
        }
        return oss.str();
    }

    // Devuelve siempre en milisegundos con 3 decimales
    std::string formatted_ms() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << milliseconds() << " ms";
        return oss.str();
    }

private:
    bool      running_;
    long long elapsed_ns_;
    TimePoint t0_;
};

#endif // TIMER_H
