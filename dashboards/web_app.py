import dash
from dash import html, dcc

import plotly.graph_objects as go
from plotly.subplots import make_subplots

from analysis import get_data, PSAR

def get_graph(symbol, psar):
    fig = make_subplots(rows=1, cols=1,
                shared_xaxes=True,
                vertical_spacing=0.10)

    ohlc_chart = data.plot(symbol=symbol).data[0]
    print(type(ohlc_chart))

    fig.add_trace(
        ohlc_chart,
        row=1, col=1
    )

    fig.add_trace(
        go.Scatter(x=psar.psar[symbol].index,
                y=psar.psar[symbol].values, 
                name="PSAR",
                mode="markers",  
                marker=dict(
                    symbol="x",  
                    size=2,      
                    color="blue"  
                )
        ),
        row=1, col=1
    )

    fig.update_layout(
        xaxis=dict(rangeslider=dict(visible=False)),  
        autosize=True,
        height=None,
        width=None
    )

    return fig


app = dash.Dash(
    __name__,
    routes_pathname_prefix='/dashboard/',
    requests_pathname_prefix='/dashboard/',
)

data = get_data()
open = data.get('Open')
high = data.get('High')
low = data.get('Low')
close = data.get('Close')

psar = PSAR.run(high, low)

fig = get_graph('BTCUSD', psar)

app.layout = html.Div([
    dcc.Graph(
        figure=fig,
        style={
            'width': '100%',
            'height': '100%' 
        }
    )
],style={'width': '95vw', 'height': '95vh', 'margin': 0, 'padding': 0})

if __name__ == '__main__':
    app.run_server(host='0.0.0.0', port=8050)