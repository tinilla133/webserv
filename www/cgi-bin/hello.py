#!/usr/bin/python3

import os
import datetime

print("Content-Type: text/html")
print("")

print("""<!DOCTYPE html>
<html>
<head>
    <title>🐍 Hola desde Python!</title>
    <meta charset="UTF-8">
    <style>
        body {
            font-family: 'Arial', sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            margin: 0;
            padding: 20px;
            color: white;
        }
        .container {
            max-width: 600px;
            margin: 0 auto;
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            padding: 30px;
            backdrop-filter: blur(10px);
        }
        h1 {
            text-align: center;
            font-size: 2.5em;
            margin-bottom: 30px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        .info-box {
            background: rgba(255,255,255,0.2);
            border-radius: 10px;
            padding: 20px;
            margin: 20px 0;
        }
        .emoji {
            font-size: 2em;
            margin-right: 10px;
        }
        a {
            color: #ffd700;
            text-decoration: none;
            font-weight: bold;
        }
        a:hover {
            text-decoration: underline;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🐍 ¡Hola desde Python CGI!</h1>
        
        <div class="info-box">
            <span class="emoji">⏰</span>
            <strong>Hora actual:</strong> """ + str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")) + """
        </div>
        
        <div class="info-box">
            <span class="emoji">🌐</span>
            <strong>Método de petición:</strong> """ + os.environ.get('REQUEST_METHOD', 'Desconocido') + """
        </div>
        
        <div class="info-box">
            <span class="emoji">🔍</span>
            <strong>Query String:</strong> """ + (os.environ.get('QUERY_STRING', 'Ninguna') or 'Ninguna') + """
        </div>
        
        <div class="info-box">
            <span class="emoji">🏠</span>
            <strong>Tu navegador:</strong> """ + os.environ.get('HTTP_USER_AGENT', 'Desconocido')[:50] + """...
        </div>
        
        <div class="info-box" style="text-align: center; margin-top: 30px;">
            <a href="/cgi.html">🔙 Volver a las pruebas</a>
        </div>
    </div>
</body>
</html>""")
