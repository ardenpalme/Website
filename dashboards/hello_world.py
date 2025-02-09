import dash
from dash import html

app = dash.Dash(
    __name__,
    routes_pathname_prefix='/dashboard/',
    requests_pathname_prefix='/dashboard/',
)

app.layout = html.Div(
    children=[
        html.Div(
            children=[
                html.H1("Title", id='header-example')  
            ]
        ),
    ],
    id='main-container'  
)

if __name__ == '__main__':
    app.run_server(host='0.0.0.0', port=8050)