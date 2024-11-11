from datetime import datetime
import pandas as pd 
import numpy as np
import requests
import os

polygon_api_key = os.getenv('POLYGON_API_KEY')  

def get_polygon_data(ticker, start_date, end_date):
    #for crypto tickers
    url = "https://api.polygon.io/v2/aggs/ticker/X:{}/range/1/day/{}/{}?".format(
        ticker,
        start_date,
        end_date)

    params = {
        'adjusted': 'true',
        'sort': 'asc',
        'apiKey': polygon_api_key }

    #print("curl ", requests.Request('GET', url, params=params).prepare().url)
    response = requests.get(url, params=params)
    if response.status_code != 200:
        print(f"Failed to fetch data. Status code: {response.status_code}")
        return None

    data = response.json()

    df = pd.DataFrame(data['results'])
    df['t'] = pd.to_datetime(df['t'], unit='ms')
    df.set_index('t', inplace=True)
    df = df.drop(columns=[col for col in {'v', 'vw', 'n'} if col in df.columns])
    df = df.astype(float)
    df.index = df.index.date
    #print(df.head())

    df.rename(columns={'o' : f'{ticker}_open',
                       'h' : f'{ticker}_high',
                       'l' : f'{ticker}_low',
                       'c' : f'{ticker}_close'}, inplace=True)

    return df