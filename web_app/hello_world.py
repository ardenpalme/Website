import dash

app = dash.Dash(
    __name__,
    routes_pathname_prefix='/dashboard/',
    requests_pathname_prefix='/dashboard/',
    serve_locally = True
)

app.layout = dash.html.Div(children='Hello World')

if __name__ == '__main__':
    app.run_server(host='0.0.0.0', port=8050, debug=False)