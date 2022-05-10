from django.urls import path
from . import views

urlpatterns = [
    path('', views.index, name='index'),
    path('getupdate/', views.getupdate, name='getupdate'),
    path('getschedule/', views.get_schedule, name='get_schedule'),
    path('schedule/', views.schedule, name='getupdate'),
    path('schedule/update/', views.schedule_update, name='schedule_update'),
    path('turnon/<int:id>', views.turnon, name='turnon'),
    path('turnoff/<int:id>', views.turnoff, name='turnoff'),
]
