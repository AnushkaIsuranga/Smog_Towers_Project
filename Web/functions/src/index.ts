import {onValueWritten} from "firebase-functions/v2/database";
import {logger} from "firebase-functions";

// Use `onValueWritten` for real-time database triggers
export const logGasData = onValueWritten(
  "/gas_readings",
  (event) => {
    const data = event.data.after.val();
    const timestamp = new Date().toISOString();

    logger.info("New gas reading:", {timestamp, data});

    // Log the data to a new node
    return event.data.after.ref.parent
      ?.child("gaslogs")
      .push({timestamp, data});
  }
);
