from django.db import models
import datetime


# We have two models in our database

class Lamps(models.Model):  # we have two instances of this model per user
    status = models.BooleanField(default=False)


class Schedule(models.Model):
    # This model stores the necessary information needed for the scheduled event to take place
    FIRST = 1
    SECOND = 2

    LAMP_CHOICES = (
        (FIRST, 'first'),
        (SECOND, 'second'),
    )

    datetime_field = models.DateTimeField(default=datetime.datetime(2022, 1, 1, 0, 0, 0, 0, None))
    desired_status = models.BooleanField()
    desired_lamp = models.IntegerField(choices=LAMP_CHOICES, default=FIRST)
    repeat_weekly = models.BooleanField(default=False)
