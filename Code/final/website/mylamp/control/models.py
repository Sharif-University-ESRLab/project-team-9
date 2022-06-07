from django.db import models
import datetime


# Create your models here.

class Lamps(models.Model):
    status = models.BooleanField(default=False)


class Schedule(models.Model):
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
