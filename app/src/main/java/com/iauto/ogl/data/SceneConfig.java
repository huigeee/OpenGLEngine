package com.iauto.ogl.data;

import android.os.Parcel;
import android.os.Parcelable;

public class SceneConfig implements Parcelable {
    private Vec3 bgColor;
    private float ambientIntensity;
    private boolean shadowEnabled;
    private float lightPosX, lightPosY, lightPosZ;
    private float shadowSoftness, shadowBias;
    private int shadowMapSize;
    private boolean shadowPCFEnabled;
    private boolean ssaoEnabled;
    private float ssaoRadius, ssaoIntensity;
    private boolean reflectionEnabled;
    private Vec3 reflectionPlaneNormal;
    private float reflectionPlaneDistance;

    public SceneConfig() {
        this(new Vec3(0.1f, 0.1f, 0.15f), 1f, false,
             10f, 15f, 10f, 0.5f, 0.05f, 2048, true,
             false, 0.5f, 1f, false, new Vec3(0f, 1f, 0f), 0f);
    }
    public SceneConfig(Vec3 bgColor, float ambientIntensity, boolean shadowEnabled,
                       float lightPosX, float lightPosY, float lightPosZ,
                       float shadowSoftness, float shadowBias, int shadowMapSize,
                       boolean shadowPCFEnabled, boolean ssaoEnabled,
                       float ssaoRadius, float ssaoIntensity, boolean reflectionEnabled,
                       Vec3 reflectionPlaneNormal, float reflectionPlaneDistance) {
        this.bgColor = bgColor; this.ambientIntensity = ambientIntensity;
        this.shadowEnabled = shadowEnabled;
        this.lightPosX = lightPosX; this.lightPosY = lightPosY; this.lightPosZ = lightPosZ;
        this.shadowSoftness = shadowSoftness; this.shadowBias = shadowBias;
        this.shadowMapSize = shadowMapSize; this.shadowPCFEnabled = shadowPCFEnabled;
        this.ssaoEnabled = ssaoEnabled; this.ssaoRadius = ssaoRadius; this.ssaoIntensity = ssaoIntensity;
        this.reflectionEnabled = reflectionEnabled;
        this.reflectionPlaneNormal = reflectionPlaneNormal; this.reflectionPlaneDistance = reflectionPlaneDistance;
    }
    protected SceneConfig(Parcel parcel) {
        ClassLoader cl = Vec3.class.getClassLoader();
        bgColor = parcel.readParcelable(cl); if (bgColor == null) bgColor = new Vec3();
        ambientIntensity = parcel.readFloat();
        shadowEnabled = parcel.readByte() != 0;
        lightPosX = parcel.readFloat(); lightPosY = parcel.readFloat(); lightPosZ = parcel.readFloat();
        shadowSoftness = parcel.readFloat(); shadowBias = parcel.readFloat(); shadowMapSize = parcel.readInt();
        shadowPCFEnabled = parcel.readByte() != 0;
        ssaoEnabled = parcel.readByte() != 0; ssaoRadius = parcel.readFloat(); ssaoIntensity = parcel.readFloat();
        reflectionEnabled = parcel.readByte() != 0;
        reflectionPlaneNormal = parcel.readParcelable(cl); if (reflectionPlaneNormal == null) reflectionPlaneNormal = new Vec3();
        reflectionPlaneDistance = parcel.readFloat();
    }

    public Vec3 getBgColor() { return bgColor; }
    public void setBgColor(Vec3 v) { this.bgColor = v; }
    public float getAmbientIntensity() { return ambientIntensity; }
    public void setAmbientIntensity(float v) { this.ambientIntensity = v; }
    public boolean getShadowEnabled() { return shadowEnabled; }
    public void setShadowEnabled(boolean v) { this.shadowEnabled = v; }
    public float getLightPosX() { return lightPosX; }
    public void setLightPosX(float v) { this.lightPosX = v; }
    public float getLightPosY() { return lightPosY; }
    public void setLightPosY(float v) { this.lightPosY = v; }
    public float getLightPosZ() { return lightPosZ; }
    public void setLightPosZ(float v) { this.lightPosZ = v; }
    public float getShadowSoftness() { return shadowSoftness; }
    public void setShadowSoftness(float v) { this.shadowSoftness = v; }
    public float getShadowBias() { return shadowBias; }
    public void setShadowBias(float v) { this.shadowBias = v; }
    public int getShadowMapSize() { return shadowMapSize; }
    public void setShadowMapSize(int v) { this.shadowMapSize = v; }
    public boolean getShadowPCFEnabled() { return shadowPCFEnabled; }
    public void setShadowPCFEnabled(boolean v) { this.shadowPCFEnabled = v; }
    public boolean getSsaoEnabled() { return ssaoEnabled; }
    public void setSsaoEnabled(boolean v) { this.ssaoEnabled = v; }
    public float getSsaoRadius() { return ssaoRadius; }
    public void setSsaoRadius(float v) { this.ssaoRadius = v; }
    public float getSsaoIntensity() { return ssaoIntensity; }
    public void setSsaoIntensity(float v) { this.ssaoIntensity = v; }
    public boolean getReflectionEnabled() { return reflectionEnabled; }
    public void setReflectionEnabled(boolean v) { this.reflectionEnabled = v; }
    public Vec3 getReflectionPlaneNormal() { return reflectionPlaneNormal; }
    public void setReflectionPlaneNormal(Vec3 v) { this.reflectionPlaneNormal = v; }
    public float getReflectionPlaneDistance() { return reflectionPlaneDistance; }
    public void setReflectionPlaneDistance(float v) { this.reflectionPlaneDistance = v; }

    @Override public void writeToParcel(Parcel dest, int flags) {
        dest.writeParcelable(bgColor, flags); dest.writeFloat(ambientIntensity);
        dest.writeByte((byte) (shadowEnabled ? 1 : 0));
        dest.writeFloat(lightPosX); dest.writeFloat(lightPosY); dest.writeFloat(lightPosZ);
        dest.writeFloat(shadowSoftness); dest.writeFloat(shadowBias); dest.writeInt(shadowMapSize);
        dest.writeByte((byte) (shadowPCFEnabled ? 1 : 0));
        dest.writeByte((byte) (ssaoEnabled ? 1 : 0)); dest.writeFloat(ssaoRadius); dest.writeFloat(ssaoIntensity);
        dest.writeByte((byte) (reflectionEnabled ? 1 : 0));
        dest.writeParcelable(reflectionPlaneNormal, flags); dest.writeFloat(reflectionPlaneDistance);
    }
    @Override public int describeContents() { return 0; }

    public static final Parcelable.Creator<SceneConfig> CREATOR = new Parcelable.Creator<SceneConfig>() {
        @Override public SceneConfig createFromParcel(Parcel source) { return new SceneConfig(source); }
        @Override public SceneConfig[] newArray(int size) { return new SceneConfig[size]; }
    };
}
