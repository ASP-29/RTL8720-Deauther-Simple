static const char web[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>RTL8720dn-Deauther</title>
    <script>
        function PostData(net_num,reason) {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                 if (this.readyState == 4 && this.status == 200) {
                    alert("Deauth Attack launched successfully");
                } else if (this.readyState == 4) {
                    alert("Deauth Attack launched successfully");
                }
            };
            xhttp.open("POST", "/deauth", true);
            xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
            xhttp.send("net_num=" + net_num + "&reason=" + reason);
        }
    </script>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            background-color: #f4f4f4;
            color: #333;
        }

        h1, h2 {
            color: #0044cc;
            text-align: center;
        }

        table {
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
        }

        table, th, td {
            border: 1px solid #ddd;
        }

        th, td {
            padding: 10px;
            text-align: left;
        }

        th {
            background-color: #0044cc;
            color: #fff;
        }

        tr:nth-child(even) {
            background-color: #f9f9f9;
        }

        tr:hover {
            background-color: #e2e2e2;
        }

        

        input[type="text"] {
            padding: 8px;
            margin: 5px 0;
            border: 1px solid #ddd;
            border-radius: 4px;
            width: calc(100% - 18px);
        }

        input[type="submit"] {
            padding: 10px 15px;
            border: none;
            border-radius: 4px;
            background-color: #0044cc;
            color: #fff;
            cursor: pointer;
            font-size: 16px;
        }

        input[type="submit"]:hover {
            background-color: #0033aa;
        }

        hr {
            border: 0;
            border-top: 1px solid #ddd;
            margin: 20px 0;
        }
    </style>
</head>
<body>
    <table border='1'>
        <tr><th>Deauth</th><th>SSID</th><th>BSSID</th><th>Channel</th><th>RSSI</th><th>Frequency</th></tr>
        %DATA_PLACE%
    </table>
    <table border='1'>
    <td><form method='post' action='/rescan'><input type='submit' value='Rescan networks'>
    </form></td>
    <td><form method='post' action='/stop'><input type='submit' value='Stop Attack'>
    </form></td>
    </table>

    
</body>
</html>
)=====";
