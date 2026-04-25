import os
from dotenv import load_dotenv

Import("env")

print("🔥 load_env.py EXECUTING")

# Load .env
load_dotenv()

def require(name):
    value = os.getenv(name)
    if not value:
        raise ValueError(f"❌ Missing env: {name}")
    print(f"[ENV] {name} = {value}")
    return value

# Load variables
wifi_ssid = require("WIFI_SSID")
wifi_password = require("WIFI_PASSWORD")
server_address = require("SERVER_ADDRESS")
server_port = require("SERVER_PORT")
server_dns_name = require("SERVER_DNS_NAME")
server_ip = require("SERVER_IP")
upload_port = require("UPLOAD_PORT")

env.Append(
    CPPDEFINES=[
        ("WIFI_SSID", env.StringifyMacro(wifi_ssid)),
        ("WIFI_PASSWORD", env.StringifyMacro(wifi_password)),
        ("SERVER_ADDRESS", env.StringifyMacro(server_address)),
        ("SERVER_PORT", server_port),
        ("SERVER_DNS_NAME", env.StringifyMacro(server_dns_name)),
        ("SERVER_IP", env.StringifyMacro(server_ip)),
        ("UPLOAD_PORT", env.StringifyMacro(upload_port))
    ]
)

print("✅ CPPDEFINES injected")

# Set upload port
env.Replace(UPLOAD_PORT=upload_port)

print(f"🚀 Upload port set to: {upload_port}")
print("🔥 load_env.py DONE\n")
