// =============================================================================
// category_chips.dart  —  Horizontal category and language filter chips
// =============================================================================
import 'package:flutter/material.dart';
import '../../../../core/theme/app_theme.dart';

class CategoryChips extends StatefulWidget {
  final String selected;
  final ValueChanged<String> onSelected;
  final bool includeLanguageFilter;

  const CategoryChips({
    super.key,
    required this.selected,
    required this.onSelected,
    this.includeLanguageFilter = false,
  });

  @override
  State<CategoryChips> createState() => _CategoryChipsState();
}

class _CategoryChipsState extends State<CategoryChips> {
  String _selectedLang = 'en';

  static const _categories = [
    {'label': 'All',           'value': ''},
    {'label': '💻 Technology', 'value': 'technology'},
    {'label': '📈 Business',   'value': 'business'},
    {'label': '🗳️ Politics',   'value': 'politics'},
    {'label': '🔬 Science',    'value': 'science'},
    {'label': '🏥 Health',     'value': 'health'},
    {'label': '🎬 Entertainment','value':'entertainment'},
    {'label': '🌿 Climate',    'value': 'climate'},
  ];

  static const _languages = [
    {'label': 'English', 'value': 'en'},
    {'label': 'हिंदी',   'value': 'hi'},
    {'label': 'తెలుగు',  'value': 'te'},
    {'label': 'தமிழ்',   'value': 'ta'},
    {'label': 'ಕನ್ನಡ',   'value': 'kn'},
    {'label': 'മലയാളം',  'value': 'ml'},
    {'label': 'मराठी',   'value': 'mr'},
    {'label': 'ગુજરાતી',  'value': 'gu'},
    {'label': 'বাংলা',   'value': 'bn'},
  ];

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // Category Chips
        SizedBox(
          height: 44,
          child: ListView.separated(
            scrollDirection: Axis.horizontal,
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
            itemCount: _categories.length,
            separatorBuilder: (_, __) => const SizedBox(width: 8),
            itemBuilder: (context, i) {
              final cat = _categories[i];
              final isSelected = widget.selected == cat['value'];
              return ChoiceChip(
                label: Text(cat['label']!),
                selected: isSelected,
                selectedColor: AppColors.primary.withOpacity(0.15),
                checkmarkColor: AppColors.primary,
                labelStyle: TextStyle(
                  color: isSelected ? AppColors.primary : null,
                  fontWeight: isSelected ? FontWeight.w600 : null,
                ),
                onSelected: (val) {
                  if (val) widget.onSelected(cat['value']!);
                },
              );
            },
          ),
        ),

        // Language Row (Optional)
        if (widget.includeLanguageFilter)
          SizedBox(
            height: 38,
            child: ListView.separated(
              scrollDirection: Axis.horizontal,
              padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
              itemCount: _languages.length,
              separatorBuilder: (_, __) => const SizedBox(width: 6),
              itemBuilder: (context, i) {
                final lang = _languages[i];
                final isSelected = _selectedLang == lang['value'];
                return InkWell(
                  onTap: () {
                    setState(() => _selectedLang = lang['value']!);
                  },
                  borderRadius: BorderRadius.circular(20),
                  child: Container(
                    padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
                    decoration: BoxDecoration(
                      color: isSelected ? AppColors.accent.withOpacity(0.2) : Colors.transparent,
                      borderRadius: BorderRadius.circular(20),
                      border: Border.all(
                        color: isSelected ? AppColors.accent : Colors.grey.withOpacity(0.3),
                        width: isSelected ? 1 : 0.5,
                      ),
                    ),
                    child: Center(
                      child: Text(
                        lang['label']!,
                        style: TextStyle(
                          fontSize: 11,
                          color: isSelected ? AppColors.accent : Colors.grey,
                          fontWeight: isSelected ? FontWeight.w600 : null,
                        ),
                      ),
                    ),
                  ),
                );
              },
            ),
          ),
      ],
    );
  }
}
