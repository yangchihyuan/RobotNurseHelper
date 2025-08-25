package tw.edu.cgu.ai.kebbi

import android.graphics.Bitmap
import androidx.camera.core.ImageProxy
import java.io.ByteArrayOutputStream
import java.nio.IntBuffer

fun imageProxyToBitmap(imageProxy: ImageProxy): Bitmap {
    val width = imageProxy.width
    val height = imageProxy.height

    // Create an IntBuffer to hold the RGBA data
    val intBuffer: IntBuffer = imageProxy.planes[0].buffer.asIntBuffer()
    val pixels = IntArray(width * height)
    intBuffer.get(pixels)

    // Create a bitmap from the array of pixels
    val bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
    bitmap.setPixels(pixels, 0, width, 0, 0, width, height)

    return bitmap
}

fun bitmapToJpegByteArray(bitmap: Bitmap, quality: Int): ByteArray {
    val stream = ByteArrayOutputStream()
    bitmap.compress(Bitmap.CompressFormat.JPEG, quality, stream)
    return stream.toByteArray()
}