package dev.wbmc.bridge.link;

/** Immutable native campaign/location snapshot. Unknown context never aliases a campaign. */
public record BridgeContext(int epoch, int scene, int encounter, String campaign) {
    public String worldId() { return "warbandcraft-" + campaign; }
    public String locationId() { return scene + ":" + encounter; }
}
