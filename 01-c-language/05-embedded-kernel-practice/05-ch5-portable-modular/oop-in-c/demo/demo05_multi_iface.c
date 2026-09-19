#include <stdio.h>
#include <stdint.h>

struct power_ops {
    int (*on)(void *dev);
    int (*off)(void *dev);
};

struct comm_ops {
    int (*send)(void *dev, const char *msg);
};

struct smart_dev {
    const char *name;
    const struct power_ops *power;
    const struct comm_ops *comm;
    void *priv;
};

static int power_on(void *dev)
{
    printf("[power] ON %s\n", (const char *)dev);
    return 0;
}

static int power_off(void *dev)
{
    printf("[power] OFF %s\n", (const char *)dev);
    return 0;
}

static int comm_send(void *dev, const char *msg)
{
    printf("[comm] %s: %s\n", (const char *)dev, msg);
    return 0;
}

static const struct power_ops default_power = { .on = power_on, .off = power_off };
static const struct comm_ops default_comm = { .send = comm_send };

int main(void)
{
    struct smart_dev sensor = {
        .name  = "SENSOR-A",
        .power = &default_power,
        .comm  = &default_comm,
        .priv  = (void *)"SENSOR-A",
    };

    sensor.power->on(sensor.priv);
    sensor.comm->send(sensor.priv, "temperature=25");
    sensor.power->off(sensor.priv);
    return 0;
}
