0.8
===
- Initial release

0.9
===
- Automatically restart EPS core if WiFi doesn't connect during a given time interval
- Fixed updating MQTT flora characteristics too often (now obeying `FLORA_PUBLISH_MIN_INTERVAL_SEC` setting)
- Fixed turning off ledstrip after boot, now state resumes to latest set by HASS
- Publishing WiFi signal obeys interval setting `WIFI_PUBLISH_MIN_INTERVAL_SEC`
- HASS discovery elimnated the availability topic from plant entities