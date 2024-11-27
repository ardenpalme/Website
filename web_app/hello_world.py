import dash

app = dash.Dash(
    __name__,
    routes_pathname_prefix='/dashboard/',
    requests_pathname_prefix='/dashboard/',
    serve_locally = True
)

app.layout = dash.html.Div(children='Strategy Development In Progress...', 
                           style={'color': '#080808', 
                                  'text-align': 'center', 
                                  'font-family': 'Inconsolata, monospace', 
                                  'font-size': '16px'})

if __name__ == '__main__':
    app.run_server(host='0.0.0.0', port=8050, debug=False)