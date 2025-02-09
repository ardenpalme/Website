from vectorbtpro import *
import os
import plotly.graph_objects as go
from plotly.subplots import make_subplots

def get_data():
    priceseries_path = '/home/ubuntu/priceseries.h5'

    data = None
    if(os.path.exists(priceseries_path)):
        data = vbt.HDFData.pull(priceseries_path)

    if(data == None or (data.index[-1].date() < pd.Timestamp.now(tz='UTC').date())):
        polygon_api_key = os.getenv('POLYGON_API_KEY')
        vbt.PolygonData.set_custom_settings(
            client_config=dict(
                api_key=polygon_api_key
            )
        )
        data = vbt.PolygonData.pull(
            ["X:BTCUSD",
            "X:ETHUSD"],
            start="2022-01-01",
            timeframe="1 day"
        )

        data = data.rename_symbols({'X:BTCUSD':'BTCUSD', 
                                    'X:ETHUSD' : 'ETHUSD'})

        data.to_hdf(priceseries_path)
    
    return data

### Permutation Entropy

@njit
def array2str(arr):
    ret = ""
    for ele in arr:
        ret += str(int(ele))
    return ret
    
@njit
def calculate_H(arr, emb_dim):
    perm_win_arr = sliding_window_view(arr,emb_dim)

    # get ordinal patterns
    permutations = List()
    for arr in perm_win_arr:
        tmp=np.argsort(arr)
        ord_pattern = np.argsort(tmp)
        val = (arr, ord_pattern)
        permutations.append(val)

    # relative frequency of each ordinal pattern
    seen_patterns = Dict.empty(key_type=types.string, value_type=types.float64)
    for perm in permutations:
        perm_ord_pat = array2str(perm[1])
        if perm_ord_pat in seen_patterns:
            seen_patterns[perm_ord_pat] += 1.0
        else:
            seen_patterns[perm_ord_pat] = 1.0
        
    # calcuate permutation entropy
    num_permutations = float(len(permutations))
    H = 0
    for ct in seen_patterns.values():
        val = float(ct)/num_permutations
        H += (np.log2(val) * val)
    H *= -1
    h_n = H / (emb_dim - 1) # PE per symbol of order n

    return H, h_n 

@njit
def get_PE(close, period, emb_dim, smooth_period):
    result = np.zeros(len(close))
    
    close_fshift1 = np.roll(close,1)
    close_fshift1[0] = np.nan
    returns = close_fshift1 - close
    
    for i in range(0, len(returns)-period):
        win_arr = returns[i:i+period]
        _, h_n = calculate_H(win_arr, emb_dim)
        result[period+i] = h_n

    conv_arr = np.ones(smooth_period, dtype=np.float64)
    res = np.convolve(result, conv_arr, 'same') / smooth_period

    return res

PE = vbt.IF(
    class_name = 'Permutation Entropy',
    short_name = 'PE',
    input_names = ['close'], 
    param_names = ['period', 'emb_dim', 'smooth_period'],
    output_names = ['H']
).with_apply_func(
    get_PE,
    takes_1d=True,
    period=30,
    emb_dim=3,
    smooth_period=5,
)

### PSAR

@njit
def get_psar(high, low, af):
    psar = np.full(high.shape[0], np.nan)
    af_max = 0.2
    local_af = af
    
    prev_psar = low[0]
    ep = high[0] 
    trend = 1 
    
    for i in range(1, high.shape[0]):     
        # Current candle indicates up-trend
        if(trend == 1): 
            psar[i] = prev_psar + local_af * (ep - prev_psar)

            # Check for trend reversal to down-trend
            if psar[i] > low[i]:  
                trend = -1
                psar[i] = ep 
                local_af = af
                ep = low[i]
            else:
                ep = max(ep, high[i])
                local_af = min(local_af + af, af_max)  
            
        # Current candle indicates down-trend
        else: 
            psar[i] = prev_psar - local_af * (prev_psar - ep)
                
            # Check for trend reversal to up-trend
            if psar[i] < high[i]:  
                trend = 1
                psar[i] = ep
                local_af = af
                ep = high[i]
            else:
                ep = min(ep, low[i])
                local_af = min(local_af + af, af_max) 
                
        prev_psar = psar[i]  
                
    return psar

PSAR = vbt.IF(
    class_name = 'Parabolic Stop and Reverse',
    short_name = 'PSAR',
    input_names = ['high', 'low'], 
    param_names = ['af'],
    output_names = ['psar']
).with_apply_func(
    get_psar,
    takes_1d=True,
    af=0.03
)