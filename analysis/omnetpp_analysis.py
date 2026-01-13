#!/usr/bin/env python3
"""
OMNeT++ Ring Network Election Algorithms Comparison
Arbitrary Election vs Anonymous Election Analysis Script
"""

import re
import sys
from typing import Dict, List, Tuple
from dataclasses import dataclass
from collections import defaultdict

# Try to import visualization libraries, but don't fail if not available
VISUALIZATION_AVAILABLE = False
try:
    import matplotlib
    matplotlib.use('Agg')  # Non-interactive backend
    import matplotlib.pyplot as plt
    import seaborn as sns
    sns.set_style("whitegrid")
    plt.rcParams['figure.figsize'] = (15, 10)
    VISUALIZATION_AVAILABLE = True
except ImportError as e:
    print(f"⚠️  Visualization libraries not available: {e}")
    print("    Script will continue without generating graphs.")
    print("    Install with: pip install 'matplotlib<3.8' 'numpy<2' seaborn\n")


@dataclass
class NodeStats:
    """Node istatistik bilgileri"""
    node_id: int
    messages_sent: int
    rounds_completed: int
    final_state: str
    is_leader: bool


@dataclass
class AlgorithmMetrics:
    """Algoritma performans metrikleri"""
    algorithm_name: str
    total_messages: int
    total_rounds: int
    leader_id: int
    node_stats: List[NodeStats]
    total_events: int
    round_details: Dict[int, List[str]]
    

class OMNetPPLogParser:
    """OMNeT++ log dosyalarını parse eden sınıf"""
    
    def __init__(self, log_file: str):
        self.log_file = log_file
        self.events = []
        self.node_stats = {}
        self.algorithm_name = ""
        
    def parse(self) -> AlgorithmMetrics:
        """Log dosyasını parse eder"""
        with open(self.log_file, 'r', encoding='utf-8') as f:
            lines = f.readlines()
        
        # Algoritma ismini tespit et
        if "Arbitrary" in lines[0]:
            self.algorithm_name = "Arbitrary Election"
        elif "Anonymous" in lines[0]:
            self.algorithm_name = "Anonymous Election"
        else:
            self.algorithm_name = "Unknown"
        
        # Event sayısını bul
        total_events = 0
        for line in lines:
            event_match = re.search(r'Event #(\d+)', line)
            if event_match:
                total_events = max(total_events, int(event_match.group(1)))
        
        # Round detaylarını topla
        round_details = defaultdict(list)
        for line in lines:
            round_match = re.search(r'round (\d+)', line.lower())
            if round_match and "INFO:" in line:
                round_num = int(round_match.group(1))
                round_details[round_num].append(line.strip())
        
        # Node istatistiklerini parse et
        node_stats = []
        in_finish_section = False
        current_stats = None
        
        for line in lines:
            # Finish section başladı mı?
            if "Calling finish() methods" in line:
                in_finish_section = True
                continue
            
            if in_finish_section:
                # Node istatistik başlangıcı
                node_start = re.search(r'Node (\d+) statistics:', line)
                if node_start:
                    # Önceki node varsa kaydet
                    if current_stats and current_stats['messages_sent'] > 0:
                        node_stats.append(NodeStats(**current_stats))
                    
                    current_stats = {
                        'node_id': int(node_start.group(1)), 
                        'messages_sent': 0, 
                        'rounds_completed': 0, 
                        'final_state': '', 
                        'is_leader': False
                    }
                    continue
                
                if current_stats is None:
                    continue
                
                # Messages Sent
                msg_match = re.search(r'Messages Sent: (\d+)', line)
                if msg_match:
                    current_stats['messages_sent'] = int(msg_match.group(1))
                    continue
                
                # Rounds Completed
                round_match = re.search(r'Rounds Completed: (\d+)', line)
                if round_match:
                    current_stats['rounds_completed'] = int(round_match.group(1))
                    continue
                
                # Final State (for Anonymous)
                state_match = re.search(r'Final State: (\w+)', line)
                if state_match:
                    current_stats['final_state'] = state_match.group(1)
                    current_stats['is_leader'] = (current_stats['final_state'] == "LEADER")
                    continue
                
                # Is Leader (for Arbitrary)
                leader_match = re.search(r'Is Leader: (\w+)', line)
                if leader_match:
                    current_stats['is_leader'] = (leader_match.group(1) == "YES")
                    current_stats['final_state'] = "LEADER" if current_stats['is_leader'] else "NON_LEADER"
                    continue
                
                # Final Leader (for Arbitrary) - bu satırı gördüğünde node tamamlanmıştır
                final_leader = re.search(r'Final Leader: (\d+)', line)
                if final_leader:
                    # Eğer henüz state belirlenmemişse
                    if not current_stats['final_state']:
                        leader_id = int(final_leader.group(1))
                        current_stats['is_leader'] = (leader_id == current_stats['node_id'])
                        current_stats['final_state'] = "LEADER" if current_stats['is_leader'] else "NON_LEADER"
                    continue
        
        # Son node'u da ekle
        if current_stats and current_stats['messages_sent'] > 0:
            node_stats.append(NodeStats(**current_stats))
        
        # Leader'ı bul
        leader_id = -1
        for stat in node_stats:
            if stat.is_leader:
                leader_id = stat.node_id
                break
        
        # Toplam mesaj ve round sayısını hesapla
        total_messages = sum(stat.messages_sent for stat in node_stats)
        max_rounds = max(stat.rounds_completed for stat in node_stats) if node_stats else 0
        
        return AlgorithmMetrics(
            algorithm_name=self.algorithm_name,
            total_messages=total_messages,
            total_rounds=max_rounds,
            leader_id=leader_id,
            node_stats=node_stats,
            total_events=total_events,
            round_details=dict(round_details)
        )


class ComparisonAnalyzer:
    """İki algoritmanın karşılaştırmalı analizini yapan sınıf"""
    
    def __init__(self, arbitrary_metrics: AlgorithmMetrics, anonymous_metrics: AlgorithmMetrics):
        self.arbitrary = arbitrary_metrics
        self.anonymous = anonymous_metrics
    
    def print_summary(self):
        """Özet karşılaştırma raporu yazdır"""
        print("="*80)
        print("RING NETWORK ELECTION ALGORITHMS - KARŞILAŞTIRMALI ANALİZ")
        print("="*80)
        print()
        
        # Genel Bilgiler
        print("📊 GENEL BİLGİLER")
        print("-"*80)
        print(f"Arbitrary Election - Seçilen Leader: Node {self.arbitrary.leader_id}")
        print(f"Anonymous Election - Seçilen Leader: Node {self.anonymous.leader_id}")
        print()
        
        # Performans Karşılaştırması
        print("⚡ PERFORMANS METRİKLERİ")
        print("-"*80)
        print(f"{'Metrik':<35} | {'Arbitrary':<15} | {'Anonymous':<15} | {'Fark':<15}")
        print("-"*80)
        
        msg_diff = self.arbitrary.total_messages - self.anonymous.total_messages
        msg_diff_pct = (msg_diff / self.arbitrary.total_messages) * 100 if self.arbitrary.total_messages > 0 else 0
        
        round_diff = self.arbitrary.total_rounds - self.anonymous.total_rounds
        round_diff_pct = (round_diff / self.arbitrary.total_rounds) * 100 if self.arbitrary.total_rounds > 0 else 0
        
        event_diff = self.arbitrary.total_events - self.anonymous.total_events
        event_diff_pct = (event_diff / self.arbitrary.total_events) * 100 if self.arbitrary.total_events > 0 else 0
        
        print(f"{'Toplam Mesaj Sayısı':<35} | {self.arbitrary.total_messages:<15} | {self.anonymous.total_messages:<15} | {msg_diff:+} ({msg_diff_pct:+.1f}%)")
        print(f"{'Tamamlanan Round Sayısı':<35} | {self.arbitrary.total_rounds:<15} | {self.anonymous.total_rounds:<15} | {round_diff:+} ({round_diff_pct:+.1f}%)")
        print(f"{'Toplam Event Sayısı':<35} | {self.arbitrary.total_events:<15} | {self.anonymous.total_events:<15} | {event_diff:+} ({event_diff_pct:+.1f}%)")
        print()
        
        # Mesaj Verimliliği
        print("📈 MESAJ VERİMLİLİĞİ ANALİZİ")
        print("-"*80)
        arb_msg_per_round = self.arbitrary.total_messages / self.arbitrary.total_rounds if self.arbitrary.total_rounds > 0 else 0
        anon_msg_per_round = self.anonymous.total_messages / self.anonymous.total_rounds if self.anonymous.total_rounds > 0 else 0
        
        print(f"Arbitrary - Round başına mesaj: {arb_msg_per_round:.2f}")
        print(f"Anonymous - Round başına mesaj: {anon_msg_per_round:.2f}")
        print(f"Verimlilik farkı: {abs(arb_msg_per_round - anon_msg_per_round):.2f} mesaj/round")
        print()
        
        # Node Bazlı Analiz
        print("🔍 NODE BAZLI DETAYLI ANALİZ")
        print("-"*80)
        print(f"{'Node ID':<10} | {'Arb Msg':<10} | {'Anon Msg':<10} | {'Arb Rounds':<12} | {'Anon Rounds':<12} | {'Arb State':<15} | {'Anon State':<15}")
        print("-"*80)
        
        num_nodes = max(len(self.arbitrary.node_stats), len(self.anonymous.node_stats))
        for i in range(num_nodes):
            arb_stat = next((s for s in self.arbitrary.node_stats if s.node_id == i), None)
            anon_stat = next((s for s in self.anonymous.node_stats if s.node_id == i), None)
            
            arb_msg = arb_stat.messages_sent if arb_stat else 0
            anon_msg = anon_stat.messages_sent if anon_stat else 0
            arb_rounds = arb_stat.rounds_completed if arb_stat else 0
            anon_rounds = anon_stat.rounds_completed if anon_stat else 0
            arb_state = arb_stat.final_state if arb_stat else "N/A"
            anon_state = anon_stat.final_state if anon_stat else "N/A"
            
            leader_marker = " 👑" if (arb_stat and arb_stat.is_leader) or (anon_stat and anon_stat.is_leader) else ""
            
            print(f"Node {i}{leader_marker:<6} | {arb_msg:<10} | {anon_msg:<10} | {arb_rounds:<12} | {anon_rounds:<12} | {arb_state:<15} | {anon_state:<15}")
        
        print()
        
        # Algoritma Özellikleri
        print("🎯 ALGORİTMA KARAKTERİSTİKLERİ")
        print("-"*80)
        print("\n📘 Arbitrary Election:")
        print("  • Her node unique bir ID'ye sahip (0-7)")
        print("  • Node'lar kendi ID'lerini komşularına gönderir")
        print("  • En yüksek ID'ye sahip node leader seçilir")
        print("  • Deterministik algoritma - her zaman aynı sonucu verir")
        print("  • Tüm node'lar tüm round'lara katılır")
        print()
        
        print("📗 Anonymous Election:")
        print("  • Node'lar unique ID'lere sahip değil (anonim)")
        print("  • Her round'da rastgele bit (0 veya 1) seçilir")
        print("  • Azınlıkta kalan node'lar pasif hale geçer")
        print("  • Probabilistik algoritma - sonuç değişebilir")
        print("  • Aktif node sayısı her round'da azalır")
        print()
        
        # Sonuç ve Öneriler
        print("💡 DEĞERLENDİRME VE SONUÇLAR")
        print("-"*80)
        
        if self.anonymous.total_messages < self.arbitrary.total_messages:
            saving = self.arbitrary.total_messages - self.anonymous.total_messages
            saving_pct = (saving / self.arbitrary.total_messages) * 100 if self.arbitrary.total_messages > 0 else 0
            print(f"✅ Anonymous Election {saving} mesaj ({saving_pct:.1f}%) daha verimli")
        elif self.anonymous.total_messages > self.arbitrary.total_messages:
            extra = self.anonymous.total_messages - self.arbitrary.total_messages
            extra_pct = (extra / self.arbitrary.total_messages) * 100 if self.arbitrary.total_messages > 0 else 0
            print(f"✅ Arbitrary Election {extra} mesaj ({extra_pct:.1f}%) daha az mesaj kullandı")
        else:
            print(f"⚖️  Her iki algoritma da eşit sayıda mesaj kullandı ({self.arbitrary.total_messages} mesaj)")
        
        if self.anonymous.total_rounds < self.arbitrary.total_rounds:
            print(f"✅ Anonymous Election daha hızlı tamamlandı ({self.anonymous.total_rounds} vs {self.arbitrary.total_rounds} round)")
        elif self.anonymous.total_rounds > self.arbitrary.total_rounds:
            print(f"✅ Arbitrary Election daha hızlı tamamlandı ({self.arbitrary.total_rounds} vs {self.anonymous.total_rounds} round)")
        else:
            print(f"⚖️  Her iki algoritma da eşit sürede tamamlandı ({self.arbitrary.total_rounds} round)")
        
        print()
        print("🔍 Kullanım Senaryoları:")
        print("  • Arbitrary Election → ID'lerin bilindiği, deterministik sonuç istenen sistemler")
        print("  • Anonymous Election → Anonim network'ler, ID atanmamış sistemler, load balancing")
        print()
        print("="*80)
    
    def generate_visualizations(self, output_file='comparison_analysis.png'):
        """Görselleştirmeler oluştur"""
        if not VISUALIZATION_AVAILABLE:
            print("\n⚠️  Görselleştirme kütüphaneleri mevcut değil, grafik oluşturulamadı.")
            print("    Yüklemek için: pip install 'matplotlib<3.8' 'numpy<2' seaborn")
            return None
        
        try:
            fig, axes = plt.subplots(2, 3, figsize=(18, 12))
            fig.suptitle('Ring Network Election Algorithms - Karşılaştırmalı Analiz', 
                         fontsize=16, fontweight='bold')
            
            # 1. Toplam Mesaj Sayısı Karşılaştırması
            ax1 = axes[0, 0]
            algorithms = ['Arbitrary', 'Anonymous']
            messages = [self.arbitrary.total_messages, self.anonymous.total_messages]
            bars1 = ax1.bar(algorithms, messages, color=['#3498db', '#e74c3c'])
            ax1.set_ylabel('Toplam Mesaj Sayısı')
            ax1.set_title('Toplam Mesaj Sayısı Karşılaştırması')
            ax1.grid(axis='y', alpha=0.3)
            
            # Bar üzerine değer yaz
            for bar in bars1:
                height = bar.get_height()
                ax1.text(bar.get_x() + bar.get_width()/2., height,
                        f'{int(height)}',
                        ha='center', va='bottom', fontweight='bold')
            
            # 2. Round Sayısı Karşılaştırması
            ax2 = axes[0, 1]
            rounds = [self.arbitrary.total_rounds, self.anonymous.total_rounds]
            bars2 = ax2.bar(algorithms, rounds, color=['#3498db', '#e74c3c'])
            ax2.set_ylabel('Round Sayısı')
            ax2.set_title('Tamamlanan Round Sayısı')
            ax2.grid(axis='y', alpha=0.3)
            
            for bar in bars2:
                height = bar.get_height()
                ax2.text(bar.get_x() + bar.get_width()/2., height,
                        f'{int(height)}',
                        ha='center', va='bottom', fontweight='bold')
            
            # 3. Node Başına Mesaj Dağılımı
            ax3 = axes[0, 2]
            num_nodes = max(len(self.arbitrary.node_stats), len(self.anonymous.node_stats))
            nodes = list(range(num_nodes))
            arb_msgs = [next((s.messages_sent for s in self.arbitrary.node_stats if s.node_id == i), 0) for i in nodes]
            anon_msgs = [next((s.messages_sent for s in self.anonymous.node_stats if s.node_id == i), 0) for i in nodes]
            
            x = range(len(nodes))
            width = 0.35
            ax3.bar([i - width/2 for i in x], arb_msgs, width, label='Arbitrary', color='#3498db')
            ax3.bar([i + width/2 for i in x], anon_msgs, width, label='Anonymous', color='#e74c3c')
            ax3.set_xlabel('Node ID')
            ax3.set_ylabel('Gönderilen Mesaj Sayısı')
            ax3.set_title('Node Başına Mesaj Dağılımı')
            ax3.set_xticks(x)
            ax3.set_xticklabels([f'N{i}' for i in nodes])
            ax3.legend()
            ax3.grid(axis='y', alpha=0.3)
            
            # 4. Node Başına Round Dağılımı
            ax4 = axes[1, 0]
            arb_rounds = [next((s.rounds_completed for s in self.arbitrary.node_stats if s.node_id == i), 0) for i in nodes]
            anon_rounds = [next((s.rounds_completed for s in self.anonymous.node_stats if s.node_id == i), 0) for i in nodes]
            
            ax4.bar([i - width/2 for i in x], arb_rounds, width, label='Arbitrary', color='#3498db')
            ax4.bar([i + width/2 for i in x], anon_rounds, width, label='Anonymous', color='#e74c3c')
            ax4.set_xlabel('Node ID')
            ax4.set_ylabel('Tamamlanan Round Sayısı')
            ax4.set_title('Node Başına Round Katılımı')
            ax4.set_xticks(x)
            ax4.set_xticklabels([f'N{i}' for i in nodes])
            ax4.legend()
            ax4.grid(axis='y', alpha=0.3)
            
            # 5. Verimlilik Metrikleri
            ax5 = axes[1, 1]
            metrics = ['Msg/Round', 'Events']
            arb_metrics = [
                self.arbitrary.total_messages / self.arbitrary.total_rounds if self.arbitrary.total_rounds > 0 else 0,
                self.arbitrary.total_events
            ]
            anon_metrics = [
                self.anonymous.total_messages / self.anonymous.total_rounds if self.anonymous.total_rounds > 0 else 0,
                self.anonymous.total_events
            ]
            
            x_metrics = range(len(metrics))
            ax5.bar([i - width/2 for i in x_metrics], arb_metrics, width, label='Arbitrary', color='#3498db')
            ax5.bar([i + width/2 for i in x_metrics], anon_metrics, width, label='Anonymous', color='#e74c3c')
            ax5.set_ylabel('Değer')
            ax5.set_title('Verimlilik Metrikleri')
            ax5.set_xticks(x_metrics)
            ax5.set_xticklabels(metrics)
            ax5.legend()
            ax5.grid(axis='y', alpha=0.3)
            
            # 6. Node State Dağılımı (Anonymous için)
            ax6 = axes[1, 2]
            states = {'PASSIVE': 0, 'ACTIVE': 0, 'LEADER': 0}
            for stat in self.anonymous.node_stats:
                states[stat.final_state] = states.get(stat.final_state, 0) + 1
            
            colors_state = ['#95a5a6', '#2ecc71', '#f39c12']
            wedges, texts, autotexts = ax6.pie(states.values(), labels=states.keys(), autopct='%1.0f%%',
                                                colors=colors_state, startangle=90)
            ax6.set_title('Anonymous Election - Final Node States')
            
            # Text'leri bold yap
            for text in texts:
                text.set_fontweight('bold')
            for autotext in autotexts:
                autotext.set_color('white')
                autotext.set_fontweight('bold')
            
            plt.tight_layout()
            plt.savefig(output_file, dpi=300, bbox_inches='tight')
            print(f"\n📊 Görselleştirmeler kaydedildi: {output_file}")
            return output_file
        except Exception as e:
            print(f"\n⚠️  Görselleştirme oluşturulurken hata: {e}")
            return None


def main():
    """Ana program"""
    if len(sys.argv) != 3:
        print("Kullanım: python omnetpp_analysis.py <arbitrary_log> <anonymous_log>")
        print("Örnek: python omnetpp_analysis.py 1omnetpp.out 2omnetpp.out")
        sys.exit(1)
    
    arbitrary_file = sys.argv[1]
    anonymous_file = sys.argv[2]
    
    print("🔍 Log dosyaları analiz ediliyor...")
    
    try:
        # Parse log files
        arbitrary_parser = OMNetPPLogParser(arbitrary_file)
        arbitrary_metrics = arbitrary_parser.parse()
        
        anonymous_parser = OMNetPPLogParser(anonymous_file)
        anonymous_metrics = anonymous_parser.parse()
        
        print("✅ Parse işlemi tamamlandı\n")
        
        # Create analyzer
        analyzer = ComparisonAnalyzer(arbitrary_metrics, anonymous_metrics)
        
        # Print comparison summary
        analyzer.print_summary()
        
        # Generate visualizations (if available)
        analyzer.generate_visualizations()
        
        print(f"✅ Analiz tamamlandı!")
        
    except FileNotFoundError as e:
        print(f"❌ Hata: Dosya bulunamadı - {e}")
        sys.exit(1)
    except Exception as e:
        print(f"❌ Beklenmeyen hata: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()