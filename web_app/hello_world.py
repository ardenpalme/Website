from flask import request
import dash

app = dash.Dash(
    __name__,
    routes_pathname_prefix='/dashboard/',
    requests_pathname_prefix='/dashboard/',
    serve_locally = True
)

app.layout = dash.html.Div(children='Hello World')

@app.server.before_request
def log_request():
    print("\n=== HTTP Request ===")
    print(f"{request.method} {request.path} HTTP/1.1")
    for header, value in request.headers:
        print(f"{header}: {value}")
    if request.data:
        print("\nBody:")
        print(request.data.decode('utf-8'))
    print("====================\n")

@app.server.after_request
def log_response(response):
    print("\n=== HTTP Response ===")
    print(f"Status: {response.status}")
    print("Headers:")
    for header, value in response.headers:
        print(f"{header}: {value}")
    print("====================\n")
    return response

if __name__ == '__main__':
    app.run_server('127.0.0.1', debug=True)  