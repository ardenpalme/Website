import ssl
from datetime import datetime
import pandas as pd 
import numpy as np
import dash
from dash import dcc, html, Input, Output
import plotly.graph_objs as go
from flask_caching import Cache
from market_data import get_polygon_data


def plot_timeseries(fig, ohlc_data, title):
    # Create the candlestick chart
    fig.add_trace(
        go.Candlestick(
            x=ohlc_data.index,
            open=ohlc_data['o'],
            high=ohlc_data['h'],
            low=ohlc_data['l'],
            close=ohlc_data['c'],
            increasing_fillcolor='#ffffff',  
            increasing_line_color='#080808',  
            decreasing_fillcolor='#080808',    
            decreasing_line_color='#080808',
            showlegend=False
        )
    )

    # Disable the bottom scroll bar (range slider)
    fig.update_layout(
        xaxis=dict(
            rangeslider=dict(visible=False),  # Disable range slider
            showgrid=False,
            color='#080808'
        ),
        yaxis=dict(showgrid=False, color='#080808'),
        paper_bgcolor='#ffffff',
        plot_bgcolor='#ffffff',
        font=dict(family='Inconsolata, monospace', color='#080808', size=10),
        margin=dict(l=10, r=10, t=10, b=10)
    )

    # Add a title banner at the top
    fig.add_annotation(
        text=title,
        xref="paper", yref="paper",
        x=0.5, y=1.0,  # Position in the chart
        showarrow=False,
        font=dict(family='Inconsolata, monospace', size=16, color='#080808'),
        align='center',
        bordercolor='#ffffff',
        borderwidth=2,
        borderpad=10,
        bgcolor='#ffffff',  # Banner background color
        opacity=0.9
    )

def plot_ema(fig, ohlc_data, window, _color):
    ohlc_data['EMA_12'] = ohlc_data['c'].ewm(span=window, adjust=False).mean()
    
    # Add the EMA-12 line to the figure
    fig.add_trace(
        go.Scatter(
            x=ohlc_data.index,
            y=ohlc_data['EMA_12'],
            mode='lines',
            line=dict(color=_color, width=2),
            name=f'EMA-{window}'
        )
    )

def create_timeseries_chart(ohlc_data, title):
    fig = go.Figure()
    plot_timeseries(fig, ohlc_data, title)
    plot_ema(fig, ohlc_data, 12, '#2596be')
    plot_ema(fig, ohlc_data, 26, '#e65f25')
    return fig

def generate_dataset(start_date, tickers):
    end_date = datetime.now().strftime("%Y-%m-%d")
    all_dates = pd.date_range(start=start_date, end=end_date, freq='D')
    
    dataset = list()
    for ticker in tickers:
        ohlc_data = get_polygon_data(ticker, start_date, end_date)
        ohlc_data.rename(columns={f'{ticker}_open' : 'o',
                                  f'{ticker}_high' : 'h',
                                  f'{ticker}_low' : 'l',
                                  f'{ticker}_close' : 'c'}, inplace=True)
        print(ohlc_data.head())
        dataset.append(ohlc_data)
    
    return dataset
    
def create_app(func_ptrs, ohlc_data_list, ticker_names):
    refresh_time_sec = 3600
    app = dash.Dash(__name__)

    # Set up caching with Flask-Caching
    cache = Cache(app.server, config={
        'CACHE_TYPE': 'SimpleCache',
        'CACHE_DEFAULT_TIMEOUT': refresh_time_sec
    })

    # Define the layout with multiple graphs in a grid
    app.layout = html.Div(
        style={'backgroundColor': '#ffffff', 'padding': '20px', 'boxSizing': 'border-box', 'font-family': 'Inconsolata'},
        children=[
            html.H2(
                "Automated Plotly Timeseries Dashboard",
                style={'color': '#080808', 
                       'text-align': 'center',
                        'font-family': 'Inconsolata, monospace',
                        'font-size': '16px'
                       }
            ),
            html.Div(
                style={
                    'display': 'grid',
                    'gridTemplateColumns': 'repeat(auto-fit, minmax(400px, 1fr))',
                    'gap': '20px',
                    'width': '100%',
                    'boxSizing': 'border-box'
                },
                children=[
                    dcc.Graph(
                        id=f'timeseries-graph-{i}',
                        config={'displayModeBar': False},
                        style={'height': '400px', 'width': '100%'}
                    )
                    for i in range(len(func_ptrs))
                ]
            ),
            dcc.Interval(
                id='interval-component',
                interval=refresh_time_sec * 1000,  # Convert seconds to milliseconds
                n_intervals=0
            )
        ]
    )

    # Cache the graph generation function
    @cache.memoize(timeout=refresh_time_sec)
    def generate_cached_figure(func_ptr, ohlc_data, ticker_name):
        print(f"Regenerating figure for {ticker_name}")
        return func_ptr(ohlc_data, ticker_name)

    # Create a callback for each graph to update automatically
    for i, (func_ptr, ohlc_data, ticker_name) in enumerate(zip(func_ptrs, ohlc_data_list, ticker_names)):
        @app.callback(
            Output(f'timeseries-graph-{i}', 'figure'),
            Input('interval-component', 'n_intervals')
        )
        def update_graph(n_intervals, func_ptr=func_ptr, ohlc_data=ohlc_data, ticker_name=ticker_name):
            return generate_cached_figure(func_ptr, ohlc_data, ticker_name)

    return app

def create_chart_1(data, title):
    return create_timeseries_chart(data, title)
def create_chart_2(data, title):
    return create_timeseries_chart(data, title)
def create_chart_3(data, title):
    return create_timeseries_chart(data, title)
def create_chart_4(data, title):
    return create_timeseries_chart(data, title)
def create_chart_5(data, title):
    return create_timeseries_chart(data, title)
def create_chart_6(data, title):
    return create_timeseries_chart(data, title)
def create_chart_7(data, title):
    return create_timeseries_chart(data, title)
def create_chart_8(data, title):
    return create_timeseries_chart(data, title)

if __name__ == '__main__':
    start_date = "2021-11-13"

    tickers = ["BTCUSD", "ETHUSD", 
               "SOLUSD", "DOGEUSD",
               "XRPUSD", "ADAUSD",
               "AVAXUSD", "XMRUSD"]

    func_ptrs = [
        create_chart_1,
        create_chart_2,
        create_chart_3,
        create_chart_4,
        create_chart_5,
        create_chart_6,
        create_chart_7,
        create_chart_8
    ]

    dataset = generate_dataset(start_date, tickers)
    app = create_app(func_ptrs, dataset, tickers)

    ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
    ssl_context.load_cert_chain(certfile="/etc/ssl/certs/www_ardenpalme_com.pem", keyfile="/etc/ssl/private/custom.key")
    app.run_server(host='0.0.0.0', port=8050, ssl_context=ssl_context) 
    #<iframe src="/dashboard" width="100%" height="800px" frameborder="0"></iframe>