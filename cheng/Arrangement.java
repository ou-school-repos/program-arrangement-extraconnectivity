import java.util.ArrayList;
import java.util.StringTokenizer;

public class Arrangement {

    public static String[] ver;
    public static int R;
    public static ArrayList<Integer> nk1ans;
    public static ArrayList<Integer> consans;
    public static ArrayList<String> ex;

    // We use R as number of vertices and K - as if K>R,
    // WLOG let the last K-R be the same for all vertices
    public static void main(String[] args) {
        R = 5;
        ver = new String[R];
        nk1ans = new ArrayList<Integer>();
        consans = new ArrayList<Integer>();
        ex = new ArrayList<String>();
        String a = "";
        String b = "";
        a += (char) ('A' + 0);
        b += (char) ('A' + R);
        for (int i = 1; i < R; i++) {
            a += (char) ('A' + i);
            b += (char) ('A' + i);
        }
        ver[0] = a;
        ver[1] = b;
        solve(2, R + 1, 0);
        for (int i = 0; i < nk1ans.size(); i++) {
            System.out.println("(" + R + "nk-" + nk1ans.get(i) + ") (n-k)-" + (nk1ans.get(i) + consans.get(i)) + ", EX: " + ex.get(i));
        }
    }

    // Recursive function for brute force solution
    public static void solve(int point, int nodl, int largchg) {
        // nodl: number of different letters used, largchg is the
        // largest index such that for some vert at that
        // index that char is different from ABCDEF.
        if (point != R) {
            ArrayList<String> newVerts = new ArrayList<String>();
            for (int i = 0; i <= point - 1; i++) {
                // This is such that we don't add the previous
                // vertices as new vertices - we will ignore the first few
                newVerts.add(ver[i]);
            }
            for (int i = 0; i <= point - 1; i++) {
                for (int j = 0; j <= nodl; j++) {
                    char cur = (char) ('A' + j);
                    if (ver[i].indexOf(cur) != -1) {
                        continue;
                    } else {
                        for (int k = 0; k <= largchg + 1; k++) {
                            String temp = ver[i].substring(0, k) + cur + ver[i].substring(k + 1);
                            if (!newVerts.contains(temp)) {
                                newVerts.add(temp);
                                ver[point] = temp;
                                solve(point + 1, Math.max(nodl, j + 1), Math.max(largchg, k));
                            }
                        }
                    }
                }
            }
        } else {
            String ans = calc();
            StringTokenizer st = new StringTokenizer(ans);
            int nk1 = Integer.parseInt(st.nextToken());
            int cons = Integer.parseInt(st.nextToken());
            if (!nk1ans.contains(nk1)) {
                nk1ans.add(nk1);
                consans.add(cons);
                String exa = "";
                for (int i = 0; i < R; i++) {
                    exa += ver[i] + " ";
                }
                ex.add(exa);
            } else {
                int poi = nk1ans.indexOf(nk1);
                if (consans.get(poi) < cons) {
                    consans.remove(poi);
                    consans.add(poi, cons);
                    String exa = "";
                    for (int i = 0; i < R; i++) {
                        exa += ver[i] + " ";
                    }
                    ex.remove(poi);
                    ex.add(poi, exa);
                }
            }
        }
    }

    // Calculates neighbor set of given set of vertices
    public static String calc() {
        int nk1coef = 0;
        int cons = 0;

        for (int i = 1; i < R; i++) {
            String cur = ver[i];
            ArrayList<String> dcverts = new ArrayList<String>();
            ArrayList<Integer> chgs = new ArrayList<Integer>();

            boolean[] isShared = new boolean[R];
            int isSharednum = 0;
            for (int j = 0; j < i; j++) {
                String cur2 = ver[j];
                int differs = 0;
                int diff1 = 0;
                int diff2 = 0;
                for (int k = 0; k < R; k++) {
                    if (cur.charAt(k) != cur2.charAt(k)) {
                        if (differs == 0) {
                            diff1 = k;
                            differs++;
                        } else if (differs == 1) {
                            diff2 = k;
                            differs++;
                        } else if (differs == 2) {
                            differs++;
                            break;
                        }
                    }
                }
                if (differs == 1) {
                    if (!isShared[diff1]) {
                        isShared[diff1] = true;
                        isSharednum++;
                        nk1coef++;
                    }
                }
                if (differs == 2) {
                    if (diff1 > diff2) {
                        int temp = diff1;
                        diff1 = diff2;
                        diff2 = temp;
                    }
                    if ((cur.charAt(diff1) != cur2.charAt(diff2))) {
                        String v = cur.substring(0, diff1) + cur2.charAt(diff1) + cur.substring(diff1 + 1);
                        if (!dcverts.contains(v)) {
                            dcverts.add(v);
                            chgs.add(diff1);
                        }
                    }
                    if ((cur.charAt(diff2) != cur2.charAt(diff1))) {
                        String v = cur.substring(0, diff2) + cur2.charAt(diff2) + cur.substring(diff2 + 1);
                        if (!dcverts.contains(v)) {
                            dcverts.add(v);
                            chgs.add(diff2);
                        }
                    }
                }
            }
            for (int n = 0; n < chgs.size(); n++) {
                if (isShared[chgs.get(n)]) {
                    chgs.remove(n);
                    dcverts.remove(n);
                    n--;
                } else if (dcverts.get(n).indexOf(dcverts.get(n).charAt(chgs.get(n)), chgs.get(n) + 1) != -1) {
                    chgs.remove(n);
                    dcverts.remove(n);
                    n--;
                }
            }
            if (i == 2) {
                for (int n = 0; n < 3; n++) {
                }
            }
            cons += dcverts.size();
            cons = (cons - isSharednum) + 1;
        }
        return nk1coef + " " + cons;
    }
}
