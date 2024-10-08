import dash
from dash import dcc, html
from dash.dependencies import Input, Output, State
import plotly.graph_objs as go
import requests
from datetime import datetime
import pytz
 
# Constants for IP and port
IP_ADDRESS = "20.206.203.162"  # Endereço IP do broker MQTT
PORT_STH = 8666  # Porta do STH (Short Term History)
DASH_HOST = "0.0.0.0"  # Define o host para permitir acesso de qualquer IP
 
# Function to get luminosity data from the API
def get_luminosity_data(lastN):
    # Monta a URL para obter os dados de luminosidade
    url = f"http://{IP_ADDRESS}:{PORT_STH}/STH/v1/contextEntities/type/Lamp/id/urn:ngsi-ld:Lamp:001/attributes/luminosity?lastN={lastN}"
    headers = {
        'fiware-service': 'smart',
        'fiware-servicepath': '/'
    }
    # Realiza a requisição GET
    response = requests.get(url, headers=headers)
    if response.status_code == 200:  # Verifica se a requisição foi bem-sucedida
        data = response.json()  # Converte a resposta para JSON
        try:
            values = data['contextResponses'][0]['contextElement']['attributes'][0]['values']  # Extrai os valores de luminosidade
            return values
        except KeyError as e:  # Trata erros de chave
            print(f"Key error: {e}")
            return []
    else:
        print(f"Error accessing {url}: {response.status_code}")  # Exibe erro caso a requisição falhe
        return []
 
# Function to convert UTC timestamps to Lisbon time
def convert_to_lisbon_time(timestamps):
    utc = pytz.utc  # Define o fuso horário UTC
    lisbon = pytz.timezone('Europe/Lisbon')  # Define o fuso horário de Lisboa
    converted_timestamps = []  # Lista para armazenar os timestamps convertidos
    for timestamp in timestamps:
        try:
            # Formata o timestamp
            timestamp = timestamp.replace('T', ' ').replace('Z', '')
            converted_time = utc.localize(datetime.strptime(timestamp, '%Y-%m-%d %H:%M:%S.%f')).astimezone(lisbon)
        except ValueError:  # Trata caso onde os milissegundos não estão presentes
            converted_time = utc.localize(datetime.strptime(timestamp, '%Y-%m-%d %H:%M:%S')).astimezone(lisbon)
        converted_timestamps.append(converted_time)  # Adiciona o timestamp convertido à lista
    return converted_timestamps
 
# Set lastN value
lastN = 10  # Define quantos pontos mais recentes obter
 
app = dash.Dash(__name__)  # Cria a aplicação Dash
 
# Layout da aplicação
app.layout = html.Div([
    html.H1('Luminosity Data Viewer'),  # Título da aplicação
    dcc.Graph(id='luminosity-graph'),  # Gráfico para exibir dados de luminosidade
    dcc.Store(id='luminosity-data-store', data={'timestamps': [], 'luminosity_values': []}),  # Armazena dados históricos
    dcc.Interval(  # Define um intervalo para atualização automática
        id='interval-component',
        interval=10*1000,  # Intervalo de 10 segundos
        n_intervals=0
    )
])
 
# Callback para atualizar os dados armazenados
@app.callback(
    Output('luminosity-data-store', 'data'),  # Atualiza os dados no dcc.Store
    Input('interval-component', 'n_intervals'),  # Input do componente de intervalo
    State('luminosity-data-store', 'data')  # Estado atual dos dados
)
def update_data_store(n, stored_data):
    # Obtém os dados de luminosidade
    data_luminosity = get_luminosity_data(lastN)
 
    if data_luminosity:
        # Extrai valores e timestamps
        luminosity_values = [float(entry['attrValue']) for entry in data_luminosity]  # Converte valores para float
        timestamps = [entry['recvTime'] for entry in data_luminosity]  # Obtém timestamps
 
        # Converte timestamps para o horário de Lisboa
        timestamps = convert_to_lisbon_time(timestamps)
 
        # Adiciona novos dados aos dados armazenados
        stored_data['timestamps'].extend(timestamps)
        stored_data['luminosity_values'].extend(luminosity_values)
 
        return stored_data  # Retorna os dados atualizados
 
    return stored_data  # Retorna dados não alterados
 
# Callback para atualizar o gráfico
@app.callback(
    Output('luminosity-graph', 'figure'),  # Atualiza o gráfico com os dados
    Input('luminosity-data-store', 'data')  # Input dos dados armazenados
)
def update_graph(stored_data):
    if stored_data['timestamps'] and stored_data['luminosity_values']:
        # Calcula a média da luminosidade
        mean_luminosity = sum(stored_data['luminosity_values']) / len(stored_data['luminosity_values'])
 
        # Cria traços para o gráfico
        trace_luminosity = go.Scatter(
            x=stored_data['timestamps'],
            y=stored_data['luminosity_values'],
            mode='lines+markers',
            name='Luminosity',
            line=dict(color='orange')
        )
        trace_mean = go.Scatter(
            x=[stored_data['timestamps'][0], stored_data['timestamps'][-1]],  # Traço para a média
            y=[mean_luminosity, mean_luminosity],
            mode='lines',
            name='Mean Luminosity',
            line=dict(color='blue', dash='dash')
        )
 
        # Cria a figura do gráfico
        fig_luminosity = go.Figure(data=[trace_luminosity, trace_mean])
 
        # Atualiza layout
        fig_luminosity.update_layout(
            title='Luminosity Over Time',  # Título do gráfico
            xaxis_title='Timestamp',  # Título do eixo X
            yaxis_title='Luminosity',  # Título do eixo Y
            hovermode='closest'  # Modo de exibição de informações ao passar o mouse
        )
 
        return fig_luminosity  # Retorna a figura do gráfico
 
    return {}  # Retorna um dicionário vazio se não houver dados
 
# Executa o servidor Dash
if __name__ == '__main__':
    app.run_server(debug=True, host=DASH_HOST, port=8050)