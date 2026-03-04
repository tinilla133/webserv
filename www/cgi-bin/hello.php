<?php
$time = date("H:i:s");
$date = date("d/m/Y");
?>
<!DOCTYPE html>
<html>
<head>
    <title>🐘 PHP Simple</title>
    <style>
        body {
            font-family: 'Arial', sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            text-align: center;
            padding: 50px 20px;
            margin: 0;
        }
        .container {
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            padding: 40px;
            backdrop-filter: blur(10px);
            display: inline-block;
            min-width: 300px;
        }
        .elephant {
            font-size: 4em;
            margin-bottom: 20px;
            animation: sway 3s ease-in-out infinite;
        }
        @keyframes sway {
            0%, 100% { transform: rotate(-5deg); }
            50% { transform: rotate(5deg); }
        }
        h1 {
            margin: 0 0 20px 0;
            color: #ffd700;
        }
        .info {
            background: rgba(255,255,255,0.2);
            border-radius: 10px;
            padding: 15px;
            margin: 15px 0;
        }
        a {
            color: #ffd700;
            text-decoration: none;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="elephant">🐘</div>
        <h1>¡PHP funcionando!</h1>
        
        <div class="info">
            📅 Fecha: <?php echo $date; ?>
        </div>
        
        <div class="info">
            🕐 Hora: <?php echo $time; ?>
        </div>
        
        <div class="info">
            🌐 Método: <?php echo $_SERVER['REQUEST_METHOD'] ?? 'GET'; ?>
        </div>
        
        <p><a href="/cgi.html">🔙 Volver</a></p>
    </div>
</body>
</html>
