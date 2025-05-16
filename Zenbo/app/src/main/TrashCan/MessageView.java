/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

package tw.edu.cgu.ai.zenbo;

import android.content.Context;
import android.graphics.Canvas;
import android.text.TextPaint;
import android.util.AttributeSet;
import android.view.View;
import android.text.StaticLayout;
import android.graphics.Color;
import android.graphics.Paint.Style;
import android.text.Layout.Alignment;


public class MessageView extends View {
  private String string = "";
  private  TextPaint tp = new TextPaint();

  public MessageView(final Context context, final AttributeSet set) {
    super(context, set);
      tp.setColor(Color.BLACK);
      tp.setStyle(Style.FILL);
      tp.setTextSize(48);
  }

  public void setString(String string) {
    this.string = string;
    postInvalidate();
  }

  @Override
  public void onDraw(final Canvas canvas) {
      super.onDraw(canvas);
      StaticLayout myStaticLayout = new StaticLayout(string, tp, canvas.getWidth(), Alignment.ALIGN_NORMAL, 1.0f, 0.0f, false);
      canvas.save();
      myStaticLayout.draw(canvas);
      canvas.restore();
  }
}
