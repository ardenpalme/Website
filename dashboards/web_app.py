import dash
from dash import html, dcc

import plotly.graph_objects as go
from plotly.subplots import make_subplots

from indicators import get_data, ADX, PSAR

def get_graph(symbol, adx, psar):
    fig = make_subplots(rows=3, cols=1,
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

    ## ADX traces

    fig.add_trace(
        go.Scatter(x=adx.adx[symbol].index,
                y=adx.adx[symbol].values, 
                name="ADX",
                line=dict(color="green")
        ),
        row=2, col=1
    )

    fig.add_trace(
        go.Scatter(x=adx.plus_di[symbol].index,
                y=adx.plus_di[symbol].values, 
                name="+ DI",
                line=dict(color="red")
        ),
        row=2, col=1
    )

    fig.add_trace(
        go.Scatter(x=adx.minus_di[symbol].index,
                y=adx.minus_di[symbol].values, 
                name="- DI",
                line=dict(color="blue")
        ),
        row=2, col=1
    )

    fig.update_layout(
        xaxis=dict(rangeslider=dict(visible=False)),  
        autosize=True,
        height=None,
        width=None
    )

    return fig


app = dash.Dash(__name__)

data = get_data()
open = data.get('Open')
high = data.get('High')
low = data.get('Low')
close = data.get('Close')

adx = ADX.run(high, low, close)
psar = PSAR.run(high, low)

fig = get_graph('BTCUSD', adx, psar)

app.layout = html.Div([
        html.Div(
            children=[
                html.H1("Title", id='header-example')
            ]
        ),

        dcc.Tabs([
            dcc.Tab(
                label='Tab one', 
                children=[
                    html.Div(
                        id='tab1_chart',
                        children = [
                            dcc.Graph(
                                figure=fig,
                                style={
                                    'width': '100%',
                                    'height': '100%' 
                                }
                            )
                        ],
                        style={
                            'display': 'flex',
                            'flex-grow': 1,
                            'justify-content': 'center',
                            'align-items': 'center',
                            'width': '100%',
                            'height' : '100vh'
                        }
                    )
                ]
            ),

            dcc.Tab(
                label='Tab two', 
                children=[
                    html.Div(
                        children = [
                            dcc.Graph(
                                figure=fig,
                                style={
                                    'width': '100%',
                                    'height': '100%' 
                                }
                            )
                        ],
                        style={
                            'display': 'flex',
                            'flex-grow': 1,
                            'justify-content': 'center',
                            'align-items': 'center',
                            'width': '100%',
                            'height' : '100vh'
                        }
                    )
                ]
            )
        ])
    ], 
    id = 'main-container'
)

if __name__ == '__main__':
    app.run_server(host='0.0.0.0', port=8050)