from flask import Flask, render_template, request, Response

app = Flask(__name__)

global global_data
global_data = {
    "temperature" : 0.0,
    "led_state" : "OFF",
}

@app.route("/")
def main_page():
    return render_template("index.html", global_data=global_data)

@app.route("/temperature/post", methods=['POST'])
def temperature_post_api():
    global global_data
    global_data["temperature"] = request.data.decode("utf-8")
    return Response(status=204)

@app.route("/led_state/post", methods=['POST'])
def led_state_post_api():
    global global_data
    global_data["led_state"] = request.data.decode("utf-8")
    return Response(status=204)

@app.route("/led_state/get", methods=['GET'])
def led_state_get_api():
    global global_data
    return global_data["led_state"]