import os
from PIL import Image, ImageOps

def prepare_photos(input_dir, output_dir, target_size=(320, 240)):
    """
    Resizes/crops images to 320x240 and saves them as 1.jpg, 2.jpg, etc.
    """
    
    # Ensure output directory exists
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    valid_extensions = ('.jpg', '.jpeg', '.png', '.webp')

    # Get a list of valid files and SORT them.
    # Sorting ensures the order is consistent (alphabetical) rather than random.
    files = [f for f in os.listdir(input_dir) if f.lower().endswith(valid_extensions)]
    files.sort()

    count = 1

    for filename in files:
        input_path = os.path.join(input_dir, filename)
        
        # Define new filename: "1.jpg", "2.jpg"
        new_filename = f"{count}.jpg"
        output_path = os.path.join(output_dir, new_filename)

        try:
            with Image.open(input_path) as img:
                # 1. Fix Orientation
                img = ImageOps.exif_transpose(img)

                # 2. Convert to RGB
                # This prevents errors if input is a PNG with transparency (RGBA),
                # because JPEGs cannot hold transparency.
                if img.mode in ('RGBA', 'LA') or (img.mode == 'P' and 'transparency' in img.info):
                    img = img.convert('RGB')

                # 3. Fit to 320x240 (Center Crop)
                img = ImageOps.fit(img, target_size, method=Image.Resampling.LANCZOS)

                # 4. Save as JPG
                img.save(output_path, 'JPEG', quality=90, optimize=True)
                print(f"Processed: {filename} -> {new_filename}")
                
                # Increment counter only after successful save
                count += 1

        except Exception as e:
            print(f"Error processing {filename}: {e}")

if __name__ == "__main__":
    # --- CONFIGURATION ---
    INPUT_FOLDER = 'photos'      
    OUTPUT_FOLDER = 'processed_photos'
    TARGET_SIZE = (320, 240)
    
    prepare_photos(INPUT_FOLDER, OUTPUT_FOLDER, TARGET_SIZE)
    print("All photos processed.")