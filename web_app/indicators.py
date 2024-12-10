from vectorbtpro import *
import os
import talib
import plotly.graph_objects as go
from plotly.subplots import make_subplots

def get_data():
    data = vbt.HDFData.pull('/home/ubuntu/Web-Server/web_app/priceseries.h5')
    if(data.index[-1].date() < pd.Timestamp.now(tz='UTC').date()):
        polygon_api_key = os.getenv('POLYGON_API_KEY')
        vbt.PolygonData.set_custom_settings(
            client_config=dict(
                api_key=polygon_api_key
            )
        )
        data = vbt.PolygonData.pull(
            ["X:BTCUSD",
            "X:ETHUSD",
            "X:SOLUSD"],
            start="2022-01-01",
            timeframe="1 day"
        )

        data = data.rename_symbols({'X:BTCUSD':'BTCUSD', 
                                    'X:ETHUSD' : 'ETHUSD', 
                                    'X:SOLUSD' : 'SOLUSD'})

        data.to_hdf('priceseries.h5')
    
    return data.get('Open'), data.get('High'), data.get('Low'), data.get('Close')

@njit
def get_dm(high, low):
    high_shifted_1 = np.roll(high, 1)
    high_shifted_1[0] = np.nan
    plus_dm_cmp = high_shifted_1 - high
    plus_dm = np.where(plus_dm_cmp > 0, plus_dm_cmp, 0)
    
    low_shifted_1 = np.roll(low, 1)
    low_shifted_1[0] = np.nan
    minus_dm_cmp = low_shifted_1 - low
    minus_dm = np.where(minus_dm_cmp > 0, minus_dm_cmp, 0)
    return plus_dm, minus_dm
    
def get_adx_di(high, low, close, di_ma_len, adx_ma_len):
    plus_dm, minus_dm = get_dm(high, low)
    atr = talib.ATR(high, low, close, di_ma_len)
    plus_di = 100 * (talib.EMA(plus_dm, di_ma_len) / atr)
    minus_di = 100 * (talib.EMA(minus_dm, di_ma_len) / atr)
    
    res = np.abs(plus_di - minus_di) / plus_di + minus_di
    adx = talib.EMA(res, adx_ma_len)
    return adx, plus_di, minus_di

ADX = vbt.IF(
    class_name = 'Average Directional Momentum Index',
    short_name = 'ADX',
    input_names = ['high', 'low', 'close'], 
    param_names = ['di_ma_len', 'adx_ma_len'],
    output_names = ['adx', 'plus_di', 'minus_di']
).with_apply_func(
    get_adx_di,
    takes_1d=True,
    di_ma_len=14,
    adx_ma_len=20
)

class ADX(ADX):
    def plot(self,
             column=None,
             high_kwargs=None,
             low_kwargs=None,
             close_kwargs=None,
             plus_di_kwargs=None,
             minus_di_kwargs=None,
             adx_kwargs=None):
        high_kwargs = high_kwargs if high_kwargs else {}
        low_kwargs = low_kwargs if low_kwargs else {}
        close_kwargs = close_kwargs if close_kwargs else {}
        
        plus_di_kwargs = plus_di_kwargs if plus_di_kwargs else {}
        minus_di_kwargs = minus_di_kwargs if minus_di_kwargs else {}
        adx_kwargs = adx_kwargs if adx_kwargs else {}

        close = self.select_col_from_obj(self.close, column).rename('Close')
        plus_di = self.select_col_from_obj(self.plus_di, column).rename('Plus DI')
        minus_di = self.select_col_from_obj(self.minus_di, column).rename('Minus DI')
        adx = self.select_col_from_obj(self.adx, column).rename('ADX')

        fig = make_subplots(rows=2, cols=1,
                    shared_xaxes=True,
                    vertical_spacing=0.02)

        fig.add_trace(
            go.Scatter(x=close.index,
                       y=close.values, 
                       name="Close",
                       **close_kwargs),
            row=1, col=1
        )
        
        fig.add_trace(
            go.Scatter(x=adx.index,
                       y=adx.values, 
                       name="ADX",
                       **adx_kwargs),
            row=2, col=1
        )
        fig.add_trace(
            go.Scatter(x=plus_di.index,
                       y=plus_di.values, 
                       name="+DI",
                       **plus_di_kwargs),
            row=2, col=1
        )
        fig.add_trace(
            go.Scatter(x=minus_di.index,
                       y=minus_di.values, 
                       name="-DI",
                       **minus_di_kwargs),
            row=2, col=1
        )
        
        
        # Add figure title
        fig.update_layout(
            height=600, width=1000,
            title_text="ADX and DI Indicator Chart"
        )
        
        return fig
