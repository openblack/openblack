# glwtool

This tool reads GLW files.
The GLW format contains the VFX information for entities such as candles, god-rays, bonfires and water mist effects.

## Examples usage

* To read a GLW file, run `read --glow-content "black_and_white\Data\Citadel\engine\main.glw"`
* To extract the contents of a GLW file to a JSON file,
  run `extract -i "black_and_white\Data\Citadel\engine\main.glw" -o "main.json"`
* To write a JSON file to the GLW file format,
  run `write -i "main.json" -o "black_and_white\Data\Citadel\engine\main.glw"`

## Example JSON output

```json
[
  {
    "size": 196,
    "unk1": 0,
    "red": 0.125,
    "green": 0.18039216101169586,
    "blue": 0.13921569287776947,
    "posX": -5.7103190422058105,
    "posY": 20.620317459106445,
    "posZ": 14.034774780273438,
    "unkX": -5.7103190422058105,
    "unkY": 20.620317459106445,
    "unkZ": 14.034774780273438,
    "unkX2": 0.0,
    "unkY2": 0.0,
    "unkZ2": 0.0,
    "unk14": 0.3583678603172302,
    "unk15": -1.0963111662931624e-07,
    "unk16": 0.9335804581642151,
    "unk17": 2.2051356296515223e-08,
    "unk18": 1.0,
    "unk19": 1.4589768682071735e-07,
    "unk20": 0.9335804581642151,
    "unk21": 3.7475590630720035e-08,
    "unk22": -0.35836783051490784,
    "unk23": -5.7103190422058105,
    "unk24": 20.620317459106445,
    "unk25": 14.034774780273438,
    "unk26": 50.0,
    "dirX": 5.605193857299268e-45,
    "dirY": -1.0,
    "dirZ": -1.0,
    "unk27": 9.0,
    "unk28": 49.0,
    "name": "candle_SIZE_0_01",
    "emitterSize": 0.0
  }
]
```
