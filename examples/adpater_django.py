# basic things that we need
import os, django.core.handlers.wsgi

# Probably you will want to put your django app settings here
os.environ['DJANGO_SETTINGS_MODULE'] = 'settings'

# Enough to start django and other code stuff
wsgi_app_handler = django.core.handlers.wsgi.WSGIHandler()

# Maybe you want to preload something: 
# all stuf that you will put here will be loaded on start up
