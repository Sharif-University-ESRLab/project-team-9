from django.http import HttpResponse, HttpResponseRedirect
from django.template import loader
from django.urls import reverse
from .models import Lamps, Schedule
import datetime
from django.views.decorators.csrf import ensure_csrf_cookie
import pytz


def index(request):
    template = loader.get_template('index.html')
    lamp1 = Lamps.objects.get(id=1)
    lamp2 = Lamps.objects.get(id=2)
    context = {
        'STATE1': 'ON' if lamp1.status else 'OFF',
        'STATE2': 'ON' if lamp2.status else 'OFF',
    }
    return HttpResponse(template.render(context, request))


@ensure_csrf_cookie
def schedule(request):
    template = loader.get_template('schedule.html')
    return HttpResponse(template.render())


def getupdate(request):
    mylamps = Lamps.objects.all().values()
    status = ""
    for lamp in mylamps:
        status += '1' if lamp['status'] else '0'

    return HttpResponse(status)


def turnon(request, id):
    lamp = Lamps.objects.get(id=id)
    lamp.status = True
    lamp.save()
    return HttpResponseRedirect(reverse('index'))


def turnoff(request, id):
    lamp = Lamps.objects.get(id=id)
    lamp.status = False
    lamp.save()
    return HttpResponseRedirect(reverse('index'))


def schedule_update(request):
    datetimeval = request.POST['datetimevalue']
    date, time = datetimeval.split('T')
    year, month, day = date.split('-')
    hour, minute = time.split(':')
    tz_IR = pytz.timezone('Asia/Tehran')
    d = datetime.datetime(int(year), int(month), int(day), int(hour), int(minute), 0, 0)
    d = tz_IR.localize(d)
    chosen_lamp = 1 if request.POST['lamp'] == "Lamp 1" else 2
    chosen_state = True if request.POST['state'] == 'ON' else False
    sch = Schedule(datetime_field=d, desired_status=chosen_state, desired_lamp=chosen_lamp)
    sch.save()
    return HttpResponseRedirect(reverse('index'))


def get_schedule(reuqest):
    schedule = Schedule.objects.all().values()
    tz_IR = pytz.timezone('Asia/Tehran')
    status = ""
    for change in schedule:
        dt_desired = change['datetime_field']
        now = datetime.datetime.now(tz_IR)
        difference = dt_desired - now

        lamp_no = '1' if change['desired_lamp'] == 1 else '2'
        lamp_status = '1' if change['desired_status'] else '0'
        status += lamp_no + lamp_status + " " +str(difference.total_seconds() * 1000) + '\n'

    return HttpResponse(status)
