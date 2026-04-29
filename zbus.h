#ifndef ZBUS_H
#define ZBUS_H

#include <systemd/sd-bus.h>
#include <stdbool.h>

// Variables que usará tu zui3.h
extern float vol_value;
extern bool needs_render;

// Callback que se ejecuta cuando DBus recibe una señal de cambio
static int on_volume_signal(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    // Al recibir la señal, simplemente marcamos que necesitamos redibujar.
    // Podríamos extraer el valor aquí, pero para simplificar, 
    // forzamos a que el siguiente frame lea el volumen.
    needs_render = true;
    return 0;
}

// Configuración del bus
static sd_bus* setup_zbus() {
    sd_bus *bus = NULL;
    int r;

    // Conexión al bus de sesión
    r = sd_bus_open_user(&bus);
    if (r < 0) return NULL;

    // SUSCRIPCIÓN: Escuchamos cambios en las propiedades de PulseAudio/PipeWire
    // org.pulseaudio.Server1 es la interfaz estándar de compatibilidad
    r = sd_bus_match_signal(bus, NULL, 
                            "org.pulseaudio.Server", 
                            "/org/pulseaudio/server_info1", 
                            "org.freedesktop.DBus.Properties", 
                            "PropertiesChanged", 
                            on_volume_signal, NULL);
    
    if (r < 0) {
        sd_bus_unref(bus);
        return NULL;
    }

    return bus;
}float GetVolumeDBus(sd_bus *bus) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    double vol = 0.0;
    int r;

    // Intentamos la ruta de PipeWire/Pulse compatible
    // Si la anterior falló, prueba con esta que es más genérica:
    r = sd_bus_get_property_trivial(bus,
                                    "org.pulse.Server",                // Opción B
                                    "/org/pulseaudio/server_info1", 
                                    "org.Pulseaudio.Server1", 
                                    "Volume",
                                    &error,
                                    'd', &vol);

    if (r < 0) {
        // Si sigue fallando, vamos a usar el comando que nunca falla pero sin popen
        // Por ahora, para que no te dé 0.5 siempre, pon un valor distinto para saber que falló
        sd_bus_error_free(&error);
        return 0.12f; 
    }

    return (float)vol;
}
#endif