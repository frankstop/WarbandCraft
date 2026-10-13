package dev.wbmc.bridge;
import java.util.*;

/** Merge only exactly adjacent boxes with identical extents on the other axes. */
public final class BoxMerge {
    private BoxMerge() {}
    public static List<float[]> merge(List<float[]> input) {
        List<float[]> boxes = new ArrayList<>();
        for (float[] b : input) boxes.add(b.clone());
        for (int axis = 0; axis < 3; axis++) {
            final int a = axis;
            boxes.sort((x,y) -> {
                for (int k=0;k<3;k++) if(k!=a) {
                    int n=Float.compare(x[k],y[k]); if(n!=0)return n;
                    n=Float.compare(x[k+3],y[k+3]); if(n!=0)return n;
                }
                return Float.compare(x[a],y[a]);
            });
            List<float[]> out = new ArrayList<>();
            for(float[] b:boxes) {
                float[] last=out.isEmpty()?null:out.getLast();
                boolean match=last!=null && last[a+3]==b[a];
                for(int k=0;k<3;k++) if(k!=a) match &= last!=null && last[k]==b[k] && last[k+3]==b[k+3];
                if(match && b[a+3]-last[a]<=256) last[a+3]=b[a+3]; else out.add(b);
            }
            boxes=out;
        }
        return boxes;
    }
}
