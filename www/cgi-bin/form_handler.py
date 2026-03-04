#!/usr/bin/env python3
import cgi

print("Content-Type: text/html\n")

form = cgi.FieldStorage()

nombre = form.getvalue("nombre", "Sin nombre")
email = form.getvalue("email", "Sin email")
mensaje = form.getvalue("mensaje", "Sin mensaje")
color = form.getvalue("color", "Sin color")

print(f"""<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<title>Resultado del Formulario</title>
<style>
body {{
  background: linear-gradient(135deg, #74b9ff, #0984e3);
  font-family: Arial, sans-serif;
  color: white;
  text-align: center;
  padding: 50px;
}}
.card {{
  background: rgba(255,255,255,0.1);
  border-radius: 15px;
  padding: 30px;
  max-width: 600px;
  margin: auto;
  backdrop-filter: blur(10px);
}}
</style>
</head>
<body>
<div class="card">
<h1>📩 Formulario recibido</h1>
<p><strong>Nombre:</strong> {nombre}</p>
<p><strong>Email:</strong> {email}</p>
<p><strong>Mensaje:</strong> {mensaje}</p>
<p><strong>Color favorito:</strong> {color}</p>
<a href="/cgi.html" style="color: #fff; text-decoration: underline;">⬅ Volver</a>
</div>
</body>
</html>""")
