import json
import os

def main():
    inventory_path = "text-inventory.json"
    replacement_path = "replacement-text.json"
    
    if not os.path.exists(inventory_path):
        print(f"Error: {inventory_path} not found!")
        return
        
    with open(inventory_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    def replace_text(text):
        # Specific replacements to avoid mismatching partial strings
        if "四六级单词训练与 C++ 桌宠伴学系统" in text:
            text = text.replace("四六级单词训练与 C++ 桌宠伴学系统", "《灵契·伴读灵》--词汇羁绊养成系统")
        if "四六级单词训练与\nC++ 桌宠伴学系统" in text:
            text = text.replace("四六级单词训练与\nC++ 桌宠伴学系统", "《灵契·伴读灵》\n——词汇羁绊养成系统")
        if "四六级单词训练与" in text:
            text = text.replace("四六级单词训练与", "《灵契·伴读灵》")
        if "C++ 桌宠伴学系统" in text:
            text = text.replace("C++ 桌宠伴学系统", "词汇羁绊养成系统")
        if "C++ 桌宠伴学" in text:
            text = text.replace("C++ 桌宠伴学", "《灵契·伴读灵》")
        if "桌宠伴学" in text:
            text = text.replace("桌宠伴学", "伴读灵")
        return text

    # Walk through slide elements
    for slide_key, shapes in data.items():
        for shape_key, shape in shapes.items():
            if "paragraphs" in shape:
                for paragraph in shape["paragraphs"]:
                    if "text" in paragraph:
                        old_txt = paragraph["text"]
                        new_txt = replace_text(old_txt)
                        paragraph["text"] = new_txt

    with open(replacement_path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)
        
    print(f"Successfully processed replacements and saved to {replacement_path}")

if __name__ == "__main__":
    main()
