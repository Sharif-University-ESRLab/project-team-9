from django.http import HttpResponse, HttpResponseRedirect
from django.template import loader
from django.urls import reverse
from .models import Lamps, Schedule
import datetime
from django.views.decorators.csrf import ensure_csrf_cookie
import pytz

repeat_weekly_weeks = 10

first_on_commands = {'کامپیوتر روشن', 'آنتیک روشن', 'لاستیک فروشان', 'لاستیک روشن', 'لامپ روشن', 'روشان',
                     'لامپ یک روشن'}
first_off_commands = {
    'لامپ خاموش', 'لامپ یک خانه موش', 'لامپ یک موش', 'لامپی که خاموش', 'لامپ یک خواب موش', 'لامپ خاموش',
    'لامپ های خاموش', '91 خاموش', 'لامپ یک خانم موش', 'لامپ h1 خاموش', 'لامپ یک خاخام اوش',
    'لامپ یک خاخام موش', 'یک خاموش', 'لامپ یخی موش', 'لامپ چراغ خاموش', 'یک کاموش', 'یک کا موش', 'لامپ ها خاموش',
    'لامپ یک خاموش'}

second_on_commands = {'لامپ دور روشن', 'دو روشن', 'دور روشن', 'دانلود روشن', 'لامپ دو روشن'}

second_off_commands = {'لپ تاپ خاموش', 'دانلود خاموش', 'دانلود خاموش', 'لامپ و خاموش', 'نام دختر موش', 'کد خاموش',
                       '32 خاموش', '12 خاموش', 'لابد خاموش', 'کامپیوتر خاموش', 'پدر خانمش', 'لامپ خاموش',
                       'لامپ دو خاموش'}

all_on_commands = {'و روشن', 'روشن', 'همه روشن'}

both_on_commands = {'هردو روشن', 'پرده روشن', 'پرتو روشن', 'اردو روشن', 'هر دو روشن'}

all_off_commands = {'خاموش', 'به خانمش', 'همه خاموش'}

both_off_commands = {'درد خاموش', 'سرد و خاموش', 'اردو خاموش', 'مرد خاموش', 'اردوخان باش', 'اردو خان باش',
                     'پرتو خاموش', 'پرده خاموش', 'طرز خاموش', 'خاموش', 'هر دو خاموش'}


def index(request):
    template = loader.get_template('index.html')
    return HttpResponse(template.render())


def manualcontrol(request):
    template = loader.get_template('manualcontrol.html')
    lamp1 = Lamps.objects.get(id=1)
    lamp2 = Lamps.objects.get(id=2)
    context = {
        'STATE1': 'روشن' if lamp1.status else 'خاموش',
        'STATE2': 'روشن' if lamp2.status else 'خاموش',
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


def change_lamp(id, state):
    lamp = Lamps.objects.get(id=id)
    lamp.status = state
    lamp.save()


def turnon(request, id):
    change_lamp(id, True)
    return HttpResponseRedirect(reverse('manualcontrol'))


def turnoff(request, id):
    change_lamp(id, False)
    return HttpResponseRedirect(reverse('manualcontrol'))


def report(request):
    # 2-digit string showing the state of the lamps
    try:
        string = str(request.body)[2:]
        lamp1_state = True if string[0] == '1' else False
        lamp2_state = True if string[1] == '1' else False
        change_lamp(1, lamp1_state)
        change_lamp(2, lamp2_state)
        return HttpResponse(string + 'Success')
    except:
        return HttpResponse(request.body)


def schedule_update(request):
    datetimeval = request.POST['datetimevalue']
    date, time = datetimeval.split('T')
    year, month, day = date.split('-')
    hour, minute = time.split(':')
    tz_IR = pytz.timezone('Asia/Tehran')
    d = datetime.datetime(int(year), int(month), int(day), int(hour), int(minute), 0, 0)
    # d = tz_IR.localize(d)
    chosen_lamp = 1 if request.POST['lamp'] == "لامپ 1" else 2
    chosen_state = True if request.POST['state'] == 'روشن' else False
    repeat = False
    if request.POST.get('checkbox1', None):
        repeat = True
    sch = Schedule(datetime_field=d, desired_status=chosen_state, desired_lamp=chosen_lamp, repeat_weekly=repeat)
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
        status += lamp_no + lamp_status + " " + str(int(difference.total_seconds() * 1000)) + " "
        if change['repeat_weekly']:
            for no_week in range(repeat_weekly_weeks - 1):
                status += lamp_no + lamp_status + " " + str(
                    int((difference.total_seconds() + 7 * 24 * 3600 * (no_week + 1)) * 1000)) + " "

    return HttpResponse(status)


def viewschedule(request):
    my_schedule = Schedule.objects.all().values()
    template = loader.get_template('view_schedule.html')
    context = {
        'myschedule': my_schedule
    }
    return HttpResponse(template.render(context, request))


def delete_schedule(request, id):
    sch = Schedule.objects.get(id=id)
    sch.delete()
    return HttpResponseRedirect(reverse('viewschedule'))


def process_voice(request):
    string = request.POST['command']
    if string in first_on_commands or string in both_on_commands or string in all_on_commands:
        change_lamp(1, True)
    if string in second_on_commands or string in both_on_commands or string in all_on_commands:
        change_lamp(2, True)
    if string in first_off_commands or string in both_off_commands or string in all_off_commands:
        change_lamp(1, False)
    if string in second_off_commands or string in both_off_commands or string in all_off_commands:
        change_lamp(2, False)

    if 'برنامه ریزی' in string:
        return schedule(request)

    return HttpResponseRedirect(reverse('index'))
