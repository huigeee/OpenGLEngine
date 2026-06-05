package com.iauto.ogl.data;

import android.os.Parcel;
import android.os.Parcelable;

public class PostProcessConfig implements Parcelable {
    private int aaMode;
    private boolean bloomEnabled;
    private float bloomIntensity, bloomThreshold;
    private boolean tonemapEnabled;
    private int tonemapOp;
    private float exposure, gammaCorrection;
    private boolean vignetteEnabled;
    private float vignetteIntensity;
    private boolean fogEnabled;
    private int fogMode;
    private float fogDensity, fogStart, fogEnd;
    private float fogR, fogG, fogB;
    private boolean dofEnabled;
    private float dofFocusDist, dofNear, dofFar;
    private boolean volumetricEnabled;
    private float volumetricDensity, volumetricScattering;
    private int volumetricSteps;
    private float volumetricMaxDistance, volumetricIntensity;
    private float volumetricColorR, volumetricColorG, volumetricColorB;

    public PostProcessConfig() {
        this(0, false, 1.5f, 0.8f, true, 1, 0.4f, 2.2f, false, 0.5f,
             false, 1, 0.25f, 1f, 8f, 0.7f, 0.75f, 0.8f,
             false, 0.5f, 2f, 20f,
             false, 0.03f, 0.75f, 48, 30f, 1f, 1f, 0.95f, 0.85f);
    }
    public PostProcessConfig(int aaMode, boolean bloomEnabled, float bloomIntensity, float bloomThreshold,
                             boolean tonemapEnabled, int tonemapOp, float exposure, float gammaCorrection,
                             boolean vignetteEnabled, float vignetteIntensity,
                             boolean fogEnabled, int fogMode, float fogDensity, float fogStart, float fogEnd,
                             float fogR, float fogG, float fogB,
                             boolean dofEnabled, float dofFocusDist, float dofNear, float dofFar,
                             boolean volumetricEnabled, float volumetricDensity, float volumetricScattering,
                             int volumetricSteps, float volumetricMaxDistance, float volumetricIntensity,
                             float volumetricColorR, float volumetricColorG, float volumetricColorB) {
        this.aaMode = aaMode; this.bloomEnabled = bloomEnabled; this.bloomIntensity = bloomIntensity; this.bloomThreshold = bloomThreshold;
        this.tonemapEnabled = tonemapEnabled; this.tonemapOp = tonemapOp; this.exposure = exposure; this.gammaCorrection = gammaCorrection;
        this.vignetteEnabled = vignetteEnabled; this.vignetteIntensity = vignetteIntensity;
        this.fogEnabled = fogEnabled; this.fogMode = fogMode; this.fogDensity = fogDensity; this.fogStart = fogStart; this.fogEnd = fogEnd;
        this.fogR = fogR; this.fogG = fogG; this.fogB = fogB;
        this.dofEnabled = dofEnabled; this.dofFocusDist = dofFocusDist; this.dofNear = dofNear; this.dofFar = dofFar;
        this.volumetricEnabled = volumetricEnabled; this.volumetricDensity = volumetricDensity; this.volumetricScattering = volumetricScattering;
        this.volumetricSteps = volumetricSteps; this.volumetricMaxDistance = volumetricMaxDistance; this.volumetricIntensity = volumetricIntensity;
        this.volumetricColorR = volumetricColorR; this.volumetricColorG = volumetricColorG; this.volumetricColorB = volumetricColorB;
    }
    protected PostProcessConfig(Parcel parcel) {
        aaMode = parcel.readInt(); bloomEnabled = parcel.readByte() != 0;
        bloomIntensity = parcel.readFloat(); bloomThreshold = parcel.readFloat();
        tonemapEnabled = parcel.readByte() != 0; tonemapOp = parcel.readInt();
        exposure = parcel.readFloat(); gammaCorrection = parcel.readFloat();
        vignetteEnabled = parcel.readByte() != 0; vignetteIntensity = parcel.readFloat();
        fogEnabled = parcel.readByte() != 0; fogMode = parcel.readInt(); fogDensity = parcel.readFloat();
        fogStart = parcel.readFloat(); fogEnd = parcel.readFloat();
        fogR = parcel.readFloat(); fogG = parcel.readFloat(); fogB = parcel.readFloat();
        dofEnabled = parcel.readByte() != 0; dofFocusDist = parcel.readFloat();
        dofNear = parcel.readFloat(); dofFar = parcel.readFloat();
        volumetricEnabled = parcel.readByte() != 0; volumetricDensity = parcel.readFloat();
        volumetricScattering = parcel.readFloat(); volumetricSteps = parcel.readInt();
        volumetricMaxDistance = parcel.readFloat(); volumetricIntensity = parcel.readFloat();
        volumetricColorR = parcel.readFloat(); volumetricColorG = parcel.readFloat(); volumetricColorB = parcel.readFloat();
    }

    // Getters
    public int getAaMode() { return aaMode; }
    public boolean getBloomEnabled() { return bloomEnabled; }
    public float getBloomIntensity() { return bloomIntensity; }
    public float getBloomThreshold() { return bloomThreshold; }
    public boolean getTonemapEnabled() { return tonemapEnabled; }
    public int getTonemapOp() { return tonemapOp; }
    public float getExposure() { return exposure; }
    public float getGammaCorrection() { return gammaCorrection; }
    public boolean getVignetteEnabled() { return vignetteEnabled; }
    public float getVignetteIntensity() { return vignetteIntensity; }
    public boolean getFogEnabled() { return fogEnabled; }
    public int getFogMode() { return fogMode; }
    public float getFogDensity() { return fogDensity; }
    public float getFogStart() { return fogStart; }
    public float getFogEnd() { return fogEnd; }
    public float getFogR() { return fogR; }
    public float getFogG() { return fogG; }
    public float getFogB() { return fogB; }
    public boolean getDofEnabled() { return dofEnabled; }
    public float getDofFocusDist() { return dofFocusDist; }
    public float getDofNear() { return dofNear; }
    public float getDofFar() { return dofFar; }
    public boolean getVolumetricEnabled() { return volumetricEnabled; }
    public float getVolumetricDensity() { return volumetricDensity; }
    public float getVolumetricScattering() { return volumetricScattering; }
    public int getVolumetricSteps() { return volumetricSteps; }
    public float getVolumetricMaxDistance() { return volumetricMaxDistance; }
    public float getVolumetricIntensity() { return volumetricIntensity; }
    public float getVolumetricColorR() { return volumetricColorR; }
    public float getVolumetricColorG() { return volumetricColorG; }
    public float getVolumetricColorB() { return volumetricColorB; }

    // Setters
    public void setAaMode(int v) { this.aaMode = v; }
    public void setBloomEnabled(boolean v) { this.bloomEnabled = v; }
    public void setBloomIntensity(float v) { this.bloomIntensity = v; }
    public void setBloomThreshold(float v) { this.bloomThreshold = v; }
    public void setTonemapEnabled(boolean v) { this.tonemapEnabled = v; }
    public void setTonemapOp(int v) { this.tonemapOp = v; }
    public void setExposure(float v) { this.exposure = v; }
    public void setGammaCorrection(float v) { this.gammaCorrection = v; }
    public void setVignetteEnabled(boolean v) { this.vignetteEnabled = v; }
    public void setVignetteIntensity(float v) { this.vignetteIntensity = v; }
    public void setFogEnabled(boolean v) { this.fogEnabled = v; }
    public void setFogMode(int v) { this.fogMode = v; }
    public void setFogDensity(float v) { this.fogDensity = v; }
    public void setFogStart(float v) { this.fogStart = v; }
    public void setFogEnd(float v) { this.fogEnd = v; }
    public void setFogR(float v) { this.fogR = v; }
    public void setFogG(float v) { this.fogG = v; }
    public void setFogB(float v) { this.fogB = v; }
    public void setDofEnabled(boolean v) { this.dofEnabled = v; }
    public void setDofFocusDist(float v) { this.dofFocusDist = v; }
    public void setDofNear(float v) { this.dofNear = v; }
    public void setDofFar(float v) { this.dofFar = v; }
    public void setVolumetricEnabled(boolean v) { this.volumetricEnabled = v; }
    public void setVolumetricDensity(float v) { this.volumetricDensity = v; }
    public void setVolumetricScattering(float v) { this.volumetricScattering = v; }
    public void setVolumetricSteps(int v) { this.volumetricSteps = v; }
    public void setVolumetricMaxDistance(float v) { this.volumetricMaxDistance = v; }
    public void setVolumetricIntensity(float v) { this.volumetricIntensity = v; }
    public void setVolumetricColorR(float v) { this.volumetricColorR = v; }
    public void setVolumetricColorG(float v) { this.volumetricColorG = v; }
    public void setVolumetricColorB(float v) { this.volumetricColorB = v; }

    @Override public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(aaMode); dest.writeByte((byte) (bloomEnabled ? 1 : 0));
        dest.writeFloat(bloomIntensity); dest.writeFloat(bloomThreshold);
        dest.writeByte((byte) (tonemapEnabled ? 1 : 0)); dest.writeInt(tonemapOp);
        dest.writeFloat(exposure); dest.writeFloat(gammaCorrection);
        dest.writeByte((byte) (vignetteEnabled ? 1 : 0)); dest.writeFloat(vignetteIntensity);
        dest.writeByte((byte) (fogEnabled ? 1 : 0)); dest.writeInt(fogMode); dest.writeFloat(fogDensity);
        dest.writeFloat(fogStart); dest.writeFloat(fogEnd);
        dest.writeFloat(fogR); dest.writeFloat(fogG); dest.writeFloat(fogB);
        dest.writeByte((byte) (dofEnabled ? 1 : 0)); dest.writeFloat(dofFocusDist);
        dest.writeFloat(dofNear); dest.writeFloat(dofFar);
        dest.writeByte((byte) (volumetricEnabled ? 1 : 0)); dest.writeFloat(volumetricDensity);
        dest.writeFloat(volumetricScattering); dest.writeInt(volumetricSteps);
        dest.writeFloat(volumetricMaxDistance); dest.writeFloat(volumetricIntensity);
        dest.writeFloat(volumetricColorR); dest.writeFloat(volumetricColorG); dest.writeFloat(volumetricColorB);
    }
    @Override public int describeContents() { return 0; }

    public static final Parcelable.Creator<PostProcessConfig> CREATOR = new Parcelable.Creator<PostProcessConfig>() {
        @Override public PostProcessConfig createFromParcel(Parcel source) { return new PostProcessConfig(source); }
        @Override public PostProcessConfig[] newArray(int size) { return new PostProcessConfig[size]; }
    };
}
