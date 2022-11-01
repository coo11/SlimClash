# -*- coding: utf-8 -*-

from subprocess import run  # subprocess.PIPE=-1
from yaml import load, SafeLoader, dump


def merge_rules(rule_providers):
    """Order:
    Custom, AdBlock,
    Lan, Telegram,
    GEOIP, ..., Global, China, Match
    """
    rules = []
    with open("./config/custom.yaml") as f:
        custom = parse(f)
    if custom and "rules" in custom:
        rules += custom["rules"]
    rp = rule_providers["rule-providers"]
    if rp["AdBlock"]:
        rules.append("RULE-SET,AdBlock,DENIED")
    if "config" in rule_providers:
        config = rule_providers["config"]
        # Order Ruleset
        for i in config:
            if not "priority" in i:
                i["priority"] = 0
        config.sort(key=lambda rule: rule["priority"], reverse=True)
        rest_ruleset = []
        # Filter inline Ruleset
        for i in config:
            if "inline" in i and i["inline"]:
                rules += generate(rp[i["name"]]["url"], i["policy"])
                del rp[i["name"]]
            else:
                rest_ruleset.append(i)
        # Append Rest Ruleset
        for i in rest_ruleset:
            rules.append(f"RULE-SET,{i['name']},{i['policy']}")
    rules.append("GEOIP,CN,DIRECT,no-resolve")
    rules.append("MATCH,MATCH")
    return rules


def parse(arg):
    output = ""
    try:
        output = load(arg, Loader=SafeLoader)
    except Exception as e:
        print(e)
    return output


def edit(rules, policy):
    for i, rule in enumerate(rules):
        if rule[:7] == "IP-CIDR":
            rules[i] = f"{rule},{policy},no-resolve"
        else:
            rules[i] = f"{rule},{policy}"
    return rules


def generate(url, policy):
    proc = run(["curl", "-fs", url], stdout=-1)
    data = proc.stdout.decode("utf-8")
    ruleset = parse(data)
    if not ruleset or not "payload" in ruleset:
        return []
    return edit(ruleset["payload"], policy)


if __name__ == "__main__":
    print("Generating config.yaml....")
    with open("output.yaml", "w+") as f:
        with open("./config/basic.yaml") as b:
            f.write(b.read())
        f.write("\n\n")
        with open("./config/proxy-groups.yaml") as pg:
            f.write(pg.read())
        f.write("\n\n")
        with open("./config/rule-providers.yaml") as rp:
            rule_providers = load(rp, Loader=SafeLoader)
            rules = merge_rules(rule_providers)
            del rule_providers["config"]
        f.write(dump(rule_providers, indent=2))
        f.write("\n\n" + "rules:\n  - " + "\n  - ".join(rules))
    print("Finished.")
