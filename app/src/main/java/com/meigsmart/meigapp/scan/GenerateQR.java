package com.meigsmart.meigapp.scan;

import java.util.Hashtable;

import android.graphics.Bitmap;

import com.google.zxing.BarcodeFormat;
import com.google.zxing.EncodeHintType;
import com.google.zxing.WriterException;
import com.google.zxing.common.BitMatrix;
import com.google.zxing.qrcode.QRCodeWriter;

/**
 * 生成二维码
 */
public class GenerateQR {
    //默认生成QR的宽和高为400px
    private int QR_width = 600;
    private int QR_height = 600;
    private Bitmap bitmap;
    private String msg;//信息内容

    public int getQR_width() {
        return QR_width;
    }

    public void setQR_width(int QR_width) {
        this.QR_width = QR_width;
    }

    public int getQR_height() {
        return QR_height;
    }

    public void setQR_height(int QR_height) {
        this.QR_height = QR_height;
    }

    public String getMsg() {
        return msg;
    }

    public void setMsg(String msg) {
        this.msg = msg;
    }


    /**
     * 生成QR图
     */
    public Bitmap createImage(String msg) {
        try {
            if (msg == null || "".equals(msg) || msg.length() < 1) {
                return null;
            }
            Hashtable<EncodeHintType, String> hints = new Hashtable<EncodeHintType, String>();
            hints.put(EncodeHintType.CHARACTER_SET, "utf-8");
            BitMatrix bitMatrix = new QRCodeWriter().encode(msg,
                    BarcodeFormat.QR_CODE, QR_width, QR_height, hints);
            int[] pixels = new int[QR_width * QR_height];
            for (int y = 0; y < QR_height; y++) {
                for (int x = 0; x < QR_width; x++) {
                    if (bitMatrix.get(x, y)) {
                        pixels[y * QR_width + x] = 0xff000000;
                    } else {
                        pixels[y * QR_width + x] = 0xffffffff;
                    }

                }
            }

            bitmap = Bitmap.createBitmap(QR_width, QR_height,
                    Bitmap.Config.ARGB_8888);

            bitmap.setPixels(pixels, 0, QR_width, 0, 0, QR_width, QR_height);

        } catch (WriterException e) {
            e.printStackTrace();
        }
        return bitmap;
    }

}
