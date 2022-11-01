## A utility to generate Clash Config.yaml

### Usage
```bash
python Config.py https://clash.subscription.link/xxxxxxxxx
# Or just update subscriptions or rules by -s [links] or -r.
# View help
python Config.py --help
```
- You can give serval **clash subscription links** with space.
- If give nothing, script will get info from `custom.yaml` (If exists).
- About `custom.yaml`:
    ```yaml
    subs:
      - https://clash.subscription.link/xxxxxxxxx
      - https://clash.subscription.link/yyyyyyyyy
      # Write your subscription links here
      # ....

    rules:
      - DOMAIN-SUFFIX,example.com,DIRECT
      - DOMAIN,www.test.com,PROXY
      # Write your custom rules here and script will combine it with rule-provider's source.
      # ....
    ```
    - In this way, just run `python Config.py` is OK.
- WARNING: Because of RULE-SET contained, you need to use Clash Premium.

### TODO
  [ ] Auto convert non-clash subscription link in Python