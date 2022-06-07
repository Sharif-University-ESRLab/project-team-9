from django.urls import path
from . import views

urlpatterns = [
    path('', views.index, name='index'),
    path('getupdate/', views.getupdate, name='getupdate'),
    path('report/', views.report, name='report_states'),
    path('getschedule/', views.get_schedule, name='get_schedule'),
    path('schedule/', views.schedule, name='getupdate'),
    path('schedule/update/', views.schedule_update, name='schedule_update'),
    path('viewschedule/', views.viewschedule, name='viewschedule'),
    path('viewschedule/delete/<int:id>', views.delete_schedule, name='deleteschedule'),
    path('manualcontrol/', views.manualcontrol, name='manualcontrol'),
    path('manualcontrol/turnon/<int:id>', views.turnon, name='turnon'),
    path('manualcontrol/turnoff/<int:id>', views.turnoff, name='turnoff'),
    path('voiceprocess/', views.process_voice, name='process_voice'),
]
