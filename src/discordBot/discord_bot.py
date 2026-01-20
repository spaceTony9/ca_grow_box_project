import discord
from discord.ext import commands, tasks
import paho.mqtt.client as mqtt
import json
from datetime import datetime
from dotenv import load_dotenv
import os

load_dotenv()

# Discord Bot Token
DISCORD_TOKEN = os.getenv("DISCORD_TOKEN")

# MQTT Settings
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC_CONTROL = "esp32/control"
MQTT_TOPIC_STATUS = "esp32/status"

# Bot setup
intents = discord.Intents.default()
intents.message_content = True
bot = commands.Bot(command_prefix='!', intents=intents)

# Store latest plant status
plant_status = {
    "temperature": None,
    "humidity": None,
    "soil_moisture": None,
    "status": None,
    "pump_enabled": None,
    "last_update": None
}

# MQTT Client
mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
status_channel_id = None

def on_mqtt_connect(client, userdata, flags, rc, properties=None):
    print(f"Connected to MQTT broker with code {rc}")
    client.subscribe(MQTT_TOPIC_STATUS)
    print(f"Subscribed to topic: {MQTT_TOPIC_STATUS}")

def on_mqtt_message(client, userdata, msg):
    global plant_status
    try:
        payload = msg.payload.decode()
        print(f"Raw MQTT payload: {payload}")
        
        if payload == "online":
            print("ESP32 is online!")
            if status_channel_id:
                channel = bot.get_channel(status_channel_id)
                if channel:
                    bot.loop.create_task(
                        channel.send("✅ **ESP32 Connected!** Plant monitor is online.")
                    )
            return
        
        data = json.loads(payload)
        print(f"Parsed MQTT message: {data}")
        
        if "event" in data:
            # Handle events
            if data["event"] == "pump_activated":
                pump_type = data.get("type", "auto")
                message = f"💧 **Pump Activated!** Your plant has been watered ({'manual' if pump_type == 'manual' else 'automatic'})."
                if status_channel_id:
                    channel = bot.get_channel(status_channel_id)
                    if channel:
                        bot.loop.create_task(channel.send(message))
            
            elif data["event"] == "pump_cooldown":
                seconds = data.get("seconds", 0)
                if status_channel_id:
                    channel = bot.get_channel(status_channel_id)
                    if channel:
                        bot.loop.create_task(
                            channel.send(f"⏳ **Pump on cooldown:** {seconds} seconds remaining. Please wait before watering again.")
                        )
            
            elif data["event"] == "pump_enabled":
                if status_channel_id:
                    channel = bot.get_channel(status_channel_id)
                    if channel:
                        bot.loop.create_task(channel.send("✅ Pump service enabled"))
            
            elif data["event"] == "pump_disabled":
                if status_channel_id:
                    channel = bot.get_channel(status_channel_id)
                    if channel:
                        bot.loop.create_task(channel.send("🛑 Pump service disabled"))
        else:
            # Regular status update
            plant_status["temperature"] = data.get("temperature")
            plant_status["humidity"] = data.get("humidity")
            plant_status["soil_moisture"] = data.get("soil_moisture")
            plant_status["status"] = data.get("status")
            plant_status["pump_enabled"] = data.get("pump_enabled", True)
            plant_status["last_update"] = datetime.now()
            print(f"Updated plant_status: {plant_status}")
            
    except json.JSONDecodeError as e:
        print(f"JSON decode error: {e}")
        print(f"Payload was: {msg.payload}")

mqtt_client.on_connect = on_mqtt_connect
mqtt_client.on_message = on_mqtt_message

@bot.event
async def on_ready():
    print(f'{bot.user} has connected to Discord!')
    mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
    mqtt_client.loop_start()
    print("MQTT client started")

@bot.command(name='status', help='Get current plant status')
async def status(ctx):
    global status_channel_id
    status_channel_id = ctx.channel.id
    
    # Request fresh status
    mqtt_client.publish(MQTT_TOPIC_CONTROL, "STATUS")
    
    # Wait up to 3 seconds for response
    import asyncio
    for i in range(6):  # 6 attempts, 500ms each = 3 seconds
        if plant_status["temperature"] is not None:
            break
        await asyncio.sleep(0.5)
    
    if plant_status["temperature"] is None:
        await ctx.send("⏳ Waiting for data from ESP32... Please try again in a moment.")
        return
    
    # Rest of your status command code...
    pump_enabled = plant_status.get("pump_enabled", True)
    pump_status = "🟢 Enabled" if pump_enabled else "🔴 Disabled"
    
    embed = discord.Embed(
        title="🌱 Plant Monitor Status",
        color=discord.Color.green() if plant_status["status"] == "OK" else discord.Color.orange(),
        timestamp=plant_status["last_update"]
    )
    
    embed.add_field(
        name="🌡️ Temperature",
        value=f"{plant_status['temperature']}°C",
        inline=True
    )
    
    embed.add_field(
        name="💨 Humidity",
        value=f"{plant_status['humidity']}%",
        inline=True
    )
    
    embed.add_field(
        name="💧 Soil Moisture",
        value=f"{plant_status['soil_moisture']}%",
        inline=True
    )
    
    status_emoji = {
        "DRY": "🔴",
        "OK": "🟢",
        "WET": "🔵"
    }
    
    embed.add_field(
        name="📊 Soil Status",
        value=f"{status_emoji.get(plant_status['status'], '⚪')} {plant_status['status']}",
        inline=True
    )
    
    embed.add_field(
        name="⚙️ Pump Service",
        value=pump_status,
        inline=True
    )
    
    embed.set_footer(text="Last updated")
    
    await ctx.send(embed=embed)

@bot.command(name='pump_on', help='Enable automatic pump service')
async def pump_on(ctx):
    mqtt_client.publish(MQTT_TOPIC_CONTROL, "PUMP_ENABLE")
    await ctx.send("✅ **Pump service ENABLED**\nAutomatic watering is now active.")

@bot.command(name='pump_off', help='Disable automatic pump service')
async def pump_off(ctx):
    result = mqtt_client.publish(MQTT_TOPIC_CONTROL, "PUMP_DISABLE")
    print(f"Published PUMP_DISABLE to MQTT, result: {result}")
    await ctx.send("🛑 **Pump service DISABLED**\nAutomatic watering is now turned off.")

@bot.command(name='water', help='Manually trigger the water pump')
async def water(ctx):
    mqtt_client.publish(MQTT_TOPIC_CONTROL, "PUMP_ON")
    await ctx.send("💧 **Manual watering command sent!**\nThe pump will activate if cooldown period has passed.")

@bot.command(name='plant', help='Show all available plant commands')
async def plant_help(ctx):
    embed = discord.Embed(
        title="🌱 Plant Monitor Commands",
        description="Control and monitor your ESP32 plant system",
        color=discord.Color.blue()
    )
    
    embed.add_field(
        name="!status",
        value="Get current temperature, humidity, and soil moisture",
        inline=False
    )
    
    embed.add_field(
        name="!water",
        value="Manually trigger the water pump (one time)",
        inline=False
    )
    
    embed.add_field(
        name="!pump_on",
        value="Enable automatic pump service",
        inline=False
    )
    
    embed.add_field(
        name="!pump_off",
        value="Disable automatic pump service",
        inline=False
    )
    
    await ctx.send(embed=embed)

bot.run(DISCORD_TOKEN)