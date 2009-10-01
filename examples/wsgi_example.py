import sys

def wsgi_app_handler(environ, start_response):
    status = '200 OK'
    response_headers = [('Content-type','text/html'), ('X-sheet-h','ololo_kod')]
    start_response(status, response_headers)
    l = '<html><div style="overflow: auto; width: 800px">%s</div><br/><div>%s</div></html>' % (str(environ), str(sys.path))
    return [l] 
