package tw.edu.cgu.ai.tabletcontroller;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;

public class ConversationAdapter extends RecyclerView.Adapter<ConversationAdapter.ViewHolder> {

    private ArrayList<ConversationMessage> messages;

    public ConversationAdapter(ArrayList<ConversationMessage> messages) {
        this.messages = messages;
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {

        TextView txtMessage;

        public ViewHolder(@NonNull View itemView) {
            super(itemView);

            txtMessage = itemView.findViewById(R.id.txtMessage);
        }
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {

        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.item_message, parent, false);

        return new ViewHolder(view);

    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {

        ConversationMessage message = messages.get(position);

        if(message.isRobot()){

            holder.txtMessage.setText("🤖 " + message.getMessage());

        }else{

            holder.txtMessage.setText("👤 " + message.getMessage());

        }

    }

    @Override
    public int getItemCount() {
        return messages.size();
    }

}