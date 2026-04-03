import os

def load_env_file():
    env_vars = {}
    env_file = os.path.join(os.getcwd(), '.env')
    print(f"[load_env.py] Looking for .env at: {env_file}")
    if os.path.exists(env_file):
        print(f"[load_env.py] .env file found!")
        with open(env_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#') and '=' in line:
                    key, value = line.split('=', 1)
                    env_vars[key.strip()] = value.strip()
                    print(f"[load_env.py] Loaded: {key.strip()} = {value.strip()}")
    else:
        print(f"[load_env.py] .env file not found!")
    return env_vars

def before_build(env, platform):
    print("[load_env.py] before_build() called")
    env_vars = load_env_file()
    
    wifi_ssid = env_vars.get("WIFI_SSID", "DefaultSSID")
    wifi_password = env_vars.get("WIFI_PASSWORD", "DefaultPassword")
    server_address = env_vars.get("SERVER_ADDRESS", "192.168.68.108")
    server_port = env_vars.get("SERVER_PORT", "3000")

    print(f"[load_env.py] Using WIFI_SSID: {wifi_ssid}")
    print(f"[load_env.py] Using WIFI_PASSWORD: {wifi_password}")
    
    env.Append(
        CPPDEFINES=[
            ("WIFI_SSID", f'"{wifi_ssid}"'),
            ("WIFI_PASSWORD", f'"{wifi_password}"'),
            ("SERVER_ADDRESS", f'"{server_address}"'),
            ("SERVER_PORT", server_port)
        ]
    )
    print("[load_env.py] CPPDEFINES added")