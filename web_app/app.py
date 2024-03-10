from flask import Flask, render_template, request, Response

app = Flask(__name__)

global global_data
global_data = {
    "temperature" : 0.0,
    "device_led_state" : "OFF",
    "cloud_led_state" : "OFF",
}

@app.route("/", methods=['GET', 'POST'])
def main_page():
    global global_data
    if request.method == "POST":
        if request.form.get("cloud_led_state_on") == "Turn LED ON":
            global_data["cloud_led_state"] = "ON"
        elif request.form.get("cloud_led_state_off") == "Turn LED OFF":
            global_data["cloud_led_state"] = "OFF"
    return render_template("index.html", global_data=global_data)

@app.route("/temperature/post", methods=['POST'])
def temperature_post_api():
    global global_data
    global_data["temperature"] = request.data.decode("utf-8")
    return Response(status=204)

@app.route("/device_led_state/post", methods=['POST'])
def device_led_state_post_api():
    global global_data
    global_data["device_led_state"] = request.data.decode("utf-8")
    return Response(status=204)

@app.route("/cloud_led_state/get", methods=['GET'])
def cloud_led_state_get_api():
    global global_data
    return global_data["cloud_led_state"]