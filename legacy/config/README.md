## A utility to generate Clash `config.yaml`

### Usage
```bash
python config.py https://clash.subscription.link/xxxxxxxxx
# Or just update subscriptions or rules by -s [links] or -r.
# View help
python config.py --help
```
- You can input serveral **clash subscription links** separated by spaces.
- If give nothing, script will get info from `custom.yaml` (If exists).
- About `custom.yaml`:
    ```yaml
    subs:
      - https://clash.subscription.link/xxxxxxxxx
      - https://www.proxyprovider.com/yyyyyyyyy
      # Write your subscription links here
      # ....

    rules:
      - DOMAIN-SUFFIX,example.com,DIRECT
      - DOMAIN,www.test.com,PROXY
      # Write your custom rules here and script will combine it with rule-provider's source.
      # ....
    ```
    - In this way, just run `python config.py`.
- WARNING: Because of RULE-SET contained, you need to use Clash Premium.

### TODO
  - [ ] ~~Auto convert non-clash subscription link in Python~~ (use [subconverter](https://github.com/tindy2013/subconverter) instead)