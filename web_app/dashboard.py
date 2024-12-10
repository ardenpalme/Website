import dash
from dash import html, dcc
from indicators import ADX, get_data

app = dash.Dash(
    __name__,
    routes_pathname_prefix='/dashboard/',
    requests_pathname_prefix='/dashboard/',
    serve_locally = True
)

open, high, low, close = get_data()

adx = ADX.run(high, low, close)
fig = adx.plot(column='BTCUSD',
         close_kwargs=dict(line_color='black'),
         adx_kwargs=dict(line_color='blue'),
         plus_di_kwargs=dict(line_color='orange'),
         minus_di_kwargs=dict(line_color='limegreen')
        )

app.layout = html.Div(
    children=[
        'ADX and DI Technical Indicator',
        dcc.Graph(figure=fig)
    ],
    style={
        'color': '#080808',
        'text-align': 'center',
        'font-family': 'Inconsolata, monospace',
        'font-size': '16px'
    }
)


if __name__ == '__main__':
    app.run_server(host='0.0.0.0', port=8050, debug=False)