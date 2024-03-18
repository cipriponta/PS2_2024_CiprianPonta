from datetime import datetime
from flask import Flask, render_template, request, Response, redirect
app = Flask(__name__)

global global_data
global_data = {
    "temperature" : 0.0,
    "device_led_state" : "OFF",
    "cloud_led_state" : "OFF",
    "message_table" : [],
    "new_message": {
        "valid": False,
        "timestamp": 0,
        "message": ""
    },
    "floods_table" : [],
    "delete_flood": {
        "valid": False,
        "timestamp": 0,
    },
}

@app.route("/", methods=["GET", "POST"])
def main_page():
    global global_data
    if request.method == "POST":
        if "Turn LED ON" == request.form.get("cloud_led_state_on"):
            global_data["cloud_led_state"] = "ON"
            return redirect("/")
        elif "Turn LED OFF" == request.form.get("cloud_led_state_off"):
            global_data["cloud_led_state"] = "OFF"
            return redirect("/")
        elif "Send Message" == request.form.get("message_send"):
            global_data["new_message"]["valid"] = True
            global_data["new_message"]["timestamp"] = int(datetime.timestamp(datetime.now()))
            global_data["new_message"]["message"] = request.form.get("message_text_box")
            return redirect("/")
        else:
            button_details = list(request.form.to_dict().items())[0]
            if "Delete Flood" == button_details[1]:
                global_data["delete_flood"]["valid"] = True
                global_data["delete_flood"]["timestamp"] = button_details[0]
                return redirect("/")
    return render_template("index.html", global_data=global_data)

@app.route("/temperature/get", methods=["GET"])
def temperature_get_api():
    global global_data
    return str(global_data["temperature"])

@app.route("/temperature/post", methods=["POST"])
def temperature_post_api():
    global global_data
    global_data["temperature"] = request.data.decode("utf-8")
    return Response(status=204)

@app.route("/device_led_state/get", methods=["GET"])
def device_led_state_get_api():
    global global_data
    return global_data["device_led_state"]

@app.route("/device_led_state/post", methods=["POST"])
def device_led_state_post_api():
    global global_data
    global_data["device_led_state"] = request.data.decode("utf-8")
    return Response(status=204)

@app.route("/cloud_led_state/get", methods=["GET"])
def cloud_led_state_get_api():
    global global_data
    return global_data["cloud_led_state"]

@app.route("/messages/post", methods=["POST"])
def messages_post_api():
    global global_data
    global_data["message_table"] = request.json
    return Response(status=204)
    
@app.route("/messages/get", methods=["GET"])
def messages_get_api():
    global global_data
    htmlString = ""
    if global_data["message_table"]:
        sorted_list = sorted(global_data["message_table"], reverse=True, key=lambda k: k["timestamp"])
        for message in sorted_list:
            htmlString = htmlString + "<tr><td>{0}</td><td>{1}</td></tr>".format(datetime.fromtimestamp(message["timestamp"]), message["message"])
    return htmlString

@app.route("/new_message/get", methods=["GET"])
def new_message_get_api():
    global global_data

    # Save message and reset the variable that stores the new message
    new_message_to_return = global_data["new_message"].copy()
    global_data["new_message"]["valid"] = False
    global_data["new_message"]["timestamp"] = 0
    global_data["new_message"]["message"] = ""

    return new_message_to_return

@app.route("/timestamp/get", methods=["GET"])
def timestamp_get_api():
    return str(int(datetime.timestamp(datetime.now())))

@app.route("/floods/post", methods=["POST"])
def floods_post_api():
    global global_data
    global_data["floods_table"] = request.json
    return Response(status=204)

@app.route("/floods/get", methods=["GET"])
def floods_get_api():
    global global_data
    htmlString = ""
    if global_data["floods_table"]:
        sorted_list = sorted(global_data["floods_table"], reverse=True)
        for timestamp in sorted_list:
            htmlString = htmlString + '<tr><td>{0}</td>' \
                                      '<td><form method="post" action="/">' \
                                      '<input type="submit" name="{1}" value="Delete Flood">' \
                                      '</form></td></tr>'.format(datetime.fromtimestamp(timestamp), timestamp)
    return htmlString

@app.route("/delete_flood/get", methods=["GET"])
def delete_flood_get_api():
    global global_data

    # Save message and reset the variable that stores the new message
    delete_flood = global_data["delete_flood"].copy()
    global_data["delete_flood"]["valid"] = False
    global_data["delete_flood"]["timestamp"] = 0

    return delete_flood